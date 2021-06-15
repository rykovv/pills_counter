#include "Arduino.h"
#include <WiFi.h>
#include "freertos/event_groups.h"
#include <Wire.h>
#include "esp_camera.h"
#include "esp_wifi.h"
#include "OneButton.h"

#include "httpd_server/httpd_server.h"
#include "device/device.h"

#include "dl_lib_matrix3d.h"
#include "ml_counter/ml_counter.h"


#define ENABLE_SSD1306  0       ///< Set 1 enable to SSD1306 OLED display feature
#define ENABLE_BUTTON   1       ///< Set 1 to enable image flip through button 34
#define ENABLE_ALARM    1       ///< Set 1 to enable use of alarm. A works as a callback when the counter value reaches alarm threashold
#define SOFTAP_MODE     0       ///< Set 1 to enable Soft Access Point WiFi mode.

#define BAUDRATE        115200  ///< Serial baudrate
#define ALARM_JSON_SIZE 128     ///< Max json string size for alarm report

#define PWDN_GPIO_NUM   26      ///< PWD camera pin
#define RESET_GPIO_NUM  -1      ///< RST camera pin
#define XCLK_GPIO_NUM   32      ///< XCLK camera pin
#define SIOD_GPIO_NUM   13      ///< SIOD camera pin
#define SIOC_GPIO_NUM   12      ///< SIOC camera pin

#define Y9_GPIO_NUM     39      ///< Y9 camera pin
#define Y8_GPIO_NUM     36      ///< Y8 camera pin
#define Y7_GPIO_NUM     23      ///< Y7 camera pin
#define Y6_GPIO_NUM     18      ///< Y6 camera pin
#define Y5_GPIO_NUM     15      ///< Y5 camera pin
#define Y4_GPIO_NUM     4       ///< Y4 camera pin
#define Y3_GPIO_NUM     14      ///< Y3 camera pin
#define Y2_GPIO_NUM     5       ///< Y2 camera pin
#define VSYNC_GPIO_NUM  27      ///< VSNC camera pin
#define HREF_GPIO_NUM   25      ///< HREF camera pin
#define PCLK_GPIO_NUM   19      ///< PCLK camera pin

#define I2C_SDA         21      ///< SDA I2C board pin
#define I2C_SCL         22      ///< SCL I2C board pin

#define AS312_PIN       33      ///< PIR Sensor pin
#define BUTTON_1        34      ///< Board button

#if !SOFTAP_MODE
#define WIFI_SSID       "SpectrumSetup-28"  ///< SSID for WiFi connection
#define WIFI_PASSWD     "heartylion234"     ///< Password for WiFi connection
#endif

/* Enabling OLED */
#if ENABLE_SSD1306
#include "SSD1306.h"
#include "OLEDDisplayUi.h"
#define SSD1306_ADDRESS   0x3c  ///< SSD1306 I2C address

SSD1306 oled(SSD1306_ADDRESS, I2C_SDA, I2C_SCL);
OLEDDisplayUi ui(&oled);
#endif

#if ENABLE_BUTTON
OneButton button1(BUTTON_1, true);
#endif

#if ENABLE_ALARM
#include "esp_http_client.h"
hw_timer_t *timer = NULL;
volatile uint8_t sample_flag = 0;

void IRAM_ATTR onTimer();
#endif

String ip;
uint8_t mac[6];
EventGroupHandle_t evGroup;

device_t device;    ///< Device status & control structure

/**
 * @brief Thread worker for unattended device mode. 
 * 
 * @details Gets next image, performs counting processing 
 *      (strategy+classification), and updates 
 *      the counter value. Throughput is ~8 fps, 
 *      340% improvement with respect to the httpd monitored 
 *      mode due to avoided RGB888 to JPG encoding.
 * 
 * @param[in] parameter Pointer to thread input args.
*/
void counting_task (void *parameter);

#if ENABLE_BUTTON
/** 
 * @brief Flip image. 
 * 
 * @details Functionality associated with the button1
*/
void button1Func() {
    static bool en = false;
    xEventGroupClearBits(evGroup, 1);
    sensor_t *s = esp_camera_sensor_get();
    en = en ? 0 : 1;
    s->set_vflip(s, en);
    delay(200);
    xEventGroupSetBits(evGroup, 1);
}
#endif

#if ENABLE_SSD1306
/**
 *  Refreshes frame 1 of the OLED display UI. Additional functionality can be added here,
 *      i.e. device's connectivity mode and IP address.
 * 
 *  @param[in] display  pointer to the display driver 
 *  @param[in] state    pointer to the UI state
 *  @param[in] x        x-coordinate associated with the writing cursor
 *  @param[in] y        y-coordinate associated with the writing cursor 
*/
void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
    display->setFont(ArialMT_Plain_16);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->drawString(64 + x, 35 + y, ip);

    if (digitalRead(AS312_PIN)) {
        display->drawString(64 + x, 10 + y, "AS312 Trigger");
    }
}

/**
 *  Refreshes frame 2 of the OLED display UI. Additional functionality can be added here,
 *      i.e. device's connectivity mode and IP address.
 *  @param[in] display  pointer to the display driver 
 *  @param[in] state    pointer to the UI state
 *  @param[in] x        x-coordinate associated with the writing cursor
 *  @param[in] y        y-coordinate associated with the writing cursor 
*/
void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    display->setFont(ArialMT_Plain_16);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(0 + x, 0 + y, "Number of pills:");
    display->drawString(0 + x, 16 + y, "UNKNOWN");
}

FrameCallback frames[] = {drawFrame1, drawFrame2};
#define FRAMES_SIZE (sizeof(frames) / sizeof(frames[0]))
#endif


void setup() {
    Serial.begin(BAUDRATE);
    Serial.setDebugOutput(true);
    Serial.println();

#if ENABLE_SSD1306
    /* config OLED display driver */
    oled.init();
    oled.setFont(ArialMT_Plain_16);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    delay(50);
    oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "TTGO Camera");
    oled.display();

    pinMode(AS312_PIN, INPUT);
#endif
    
    /* Create Event Group */
    if (!(evGroup = xEventGroupCreate())) {
        ESP_LOGE(TAG, "evGroup Fail");
        while (1);
    }
    xEventGroupSetBits(evGroup, 1);

    /* config camera struct init */
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_240X240;
    config.jpeg_quality = 10;
    config.fb_count = 2;

    /* camera sensor init */
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGD(TAG, "Camera init Fail");
#if ENABLE_SSD1306
        oled.clear();
        oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "Camera init Fail");
        oled.display();
#endif
        while (1);
    }

    /* Additional setup for camera sensor */
    sensor_t *s = esp_camera_sensor_get();
    /* Flip image initially if necessary */
    // s->set_vflip(s, 1);
    /* Black and white special effect set */
    s->set_special_effect(s, 2);

    /* Attach button1 functionality */
    #if ENABLE_BUTTON
    button1.attachClick(button1Func);
    #endif

    #if SOFTAP_MODE
    /* Setup AP */
    char buff[128];
    WiFi.mode(WIFI_AP);
    IPAddress apIP = IPAddress(2, 2, 2, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    esp_wifi_get_mac(WIFI_IF_AP, mac);
    snprintf(buff, "PILLS-COUNTER-%02X:%02X", 128 mac[4], mac[5]);
    ESP_LOGI(TAG, "Device AP Name:%s\n", buff);
    if (!WiFi.softAP(buff, NULL, 1, 0)) {
        ESP_LOGE(TAG, "AP Begin Failed.");
        while (1);
    }
    #else
    /* Connect to WiFi */
    WiFi.begin(WIFI_SSID, WIFI_PASSWD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    #endif

#if ENABLE_SSD1306
    oled.clear();
    oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "WiFi Connected");
    oled.display();
#endif

    /* Moving average struct intialization */
    moving_avg_init(&device.stats.ma, MOVING_AVG_SAMPLES);

    /* Set detline vector to default values */
    ml_counter_init(device.shared.detline);

    /* Start httpd monitor server */
    httpd_server_init(&device);

    delay(50);

#if ENABLE_SSD1306
    /* Setup UI */
    ui.setTargetFPS(60);
    ui.setIndicatorPosition(BOTTOM);
    ui.setIndicatorDirection(LEFT_RIGHT);
    ui.setFrameAnimation(SLIDE_LEFT);
    ui.setFrames(frames, FRAMES_SIZE);
    ui.setTimePerFrame(6000);
    ui.init();
#endif

#if SOFTAP_MODE
    ip = WiFi.softAPIP().toString();
    Serial.printf("\nAp Started .. Please Connect %s hotspot\n", buff);
#else
    ip = WiFi.localIP().toString();
#endif

    Serial.print("Pills Counter Ready! Use 'http://");
    Serial.print(ip);
    Serial.println("' to connect");

#if ENABLE_ALARM
    /* Setup timer and associated interrupt */
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000000, true); // check counter every second
    timerAlarmEnable(timer);
#endif

    /* Create counting worker for unattended mode */
    xTaskCreate (
        counting_task,          /* Task function. */
        "counting_task",        /* String with name of task. */
        5120,                   /* Stack size in bytes. */
        NULL,                   /* Parameter passed as input of the task */
        1,                      /* Priority of the task. */
        NULL                    /* Task handle. */
    );                  
}

void loop () {
    #if ENABLE_SSD1306
    if (ui.update()) {
#endif
#if ENABLE_BUTTON
        button1.tick();
#endif
#if ENABLE_SSD1306
    }
#endif
#if ENABLE_ALARM
    /* Alarm check */
    if (sample_flag) {
        /* Send JSON alarm if condition is met */
        if (device.status.alarm_enable && device.status.counter_value >= device.status.alarm_count
                                        && strnlen(device.status.alarm_link, ALARM_LINK_MAX_SIZE) > 10) 
        {
            // url correct form -> "http://<ip>:<port>/<path>/"
            esp_http_client_config_t config = {.url = device.status.alarm_link};

            char *alarm_json = (char *)malloc(sizeof(char)*ALARM_JSON_SIZE);
            esp_wifi_get_mac(WIFI_IF_AP, mac);           
            snprintf(   alarm_json, 
                        ALARM_JSON_SIZE, 
                        "{\"id\":\"PILLS-COUNTER-%02X:%02X\",\"counter\":%llu, \"alarm\":1}",
                        mac[4], mac[5], 
                        device.status.counter_value
            );
            esp_http_client_handle_t client = esp_http_client_init(&config);
            esp_http_client_set_method(client, HTTP_METHOD_POST);
            esp_http_client_set_post_field(client, alarm_json, strnlen(alarm_json, ALARM_JSON_SIZE));
            esp_http_client_set_header(client, "Content-Type", "application/json");

            esp_err_t err = esp_http_client_perform(client);

            if (err == ESP_OK) {
            ESP_LOGI(TAG, "Status = %d, content_length = %d",
                    esp_http_client_get_status_code(client),
                    esp_http_client_get_content_length(client));
            }
            esp_http_client_cleanup(client);
            device.status.alarm_enable = 0;
            free(alarm_json);
        }
        sample_flag = 0;
    }
#endif
}

void counting_task (void *parameter) {
    size_t fb_len = 0;

    if(!device.stats.last_frame) {
        device.stats.last_frame = esp_timer_get_time();
    }

    for (;;) {
        if (!device.httpd_monitored) {
            device.shared.fb = esp_camera_fb_get();
            if (!device.shared.fb) {
                ESP_LOGE(TAG, "Camera capture failed");
                device.shared.fb = NULL;
            } else {
                device.stats.fr_start = esp_timer_get_time();

                if (device.shared.fb->width <= 400 && device.status.counter_enable) {
                    device.shared.image_matrix = dl_matrix3du_alloc(1, device.shared.fb->width, device.shared.fb->height, 3);

                    if (!device.shared.image_matrix) {
                        ESP_LOGE(TAG, "dl_matrix3du_alloc failed");
                    } else {
                        fb_len = device.shared.fb->len;
                        if (!fmt2rgb888(device.shared.fb->buf, device.shared.fb->len, device.shared.fb->format, device.shared.image_matrix->item)) {
                            ESP_LOGE(TAG, "fmt2rgb888 failed");
                        } else {
                            device.stats.fr_decode = esp_timer_get_time();

                            if (device.status.ca == CA_ONE_LINE_DETECTION_HYS_1) {
                                device.status.counter_value += one_detection_line_hys1(device.shared.image_matrix, *device.shared.detline);
                            } else if (device.status.ca == CA_GRID_DIVISION) {
                                device.status.counter_value = grid_division(device.shared.image_matrix);
                            } else if (device.status.ca == CA_ONE_LINE_DETECTION) {
                                device.status.counter_value += one_detection_line(device.shared.image_matrix, *device.shared.detline);
                            } else if (device.status.ca == CA_TWO_LINES_DETECTION) {
                                device.status.counter_value += two_detection_lines(device.shared.image_matrix, device.shared.detline);
                            } else {
                                // one that works best
                                device.status.counter_value += one_detection_line_hys1(device.shared.image_matrix, *device.shared.detline);
                            }
                            
                            device.stats.fr_detection = esp_timer_get_time();
                        }
                        dl_matrix3du_free(device.shared.image_matrix);
                    }
                }
                esp_camera_fb_return(device.shared.fb);
                device.shared.fb = NULL;

                if (device.status.counter_enable) {
                    int64_t fr_end = esp_timer_get_time();
                    int64_t conv_rgb888_time = (device.stats.fr_decode - device.stats.fr_start)/1000;
                    int64_t detection_time = (device.stats.fr_detection - device.stats.fr_decode)/1000;
                    int64_t process_time = (device.stats.fr_detection - device.stats.fr_start)/1000;
                    int64_t frame_time = fr_end - device.stats.last_frame;
                    device.stats.last_frame = fr_end;
                    frame_time /= 1000;
                    uint32_t avg_frame_time = moving_avg_run(&device.stats.ma, frame_time);
                    
                    ESP_LOGD(TAG, "UNAT: %uB %ums (%.1ffps), AVG: %ums (%.1ffps), %u+%u=%u\n",
                        (uint32_t)(fb_len),
                        (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time,
                        avg_frame_time, 1000.0 / avg_frame_time,
                        (uint32_t)conv_rgb888_time, (uint32_t)detection_time, (uint32_t)process_time
                    );
                }
            }
        }
    }
}

#if ENABLE_ALARM
void IRAM_ATTR onTimer() {
    sample_flag = 1;
}
#endif