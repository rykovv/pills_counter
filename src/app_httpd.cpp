#include "Arduino.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "dl_lib_matrix3d.h"

#include "pills_index.h"

#include "model.h"

#define ALARM_LINK_MAX_SIZE     30

// should match with the html 
typedef enum {
    CA_GRID_DIVISION = 0,
    CA_ONE_LINE_DETECTION,
    CA_ONE_LINE_DETECTION_HYS_1,
    CA_TWO_LINES_DETECTION
} counting_algorithm_t;

typedef struct {
    uint8_t counter_enable = 0;
    counting_algorithm_t ca = CA_ONE_LINE_DETECTION_HYS_1;
    uint8_t alarm_enable = 0;
    uint64_t alarm_count = 0;
    char alarm_link[ALARM_LINK_MAX_SIZE];
} counter_status_t;

typedef enum {
    PILL_NO = 0,
    PILL_DETECTED,
    PILL_DISAPPEARED,
    PILL_DISAPPEARED_HYS_1,
    PILL_EXPECTED
} pill_state_t;

typedef struct {
        size_t size; //number of values used for filtering
        size_t index; //current value index
        size_t count; //value count
        int sum;
        int * values; //array to be filled with values
} ra_filter_t;

typedef struct {
        httpd_req_t *req;
        size_t len;
} jpg_chunking_t;

#define SUBFRAME_WIDTH  24
#define SUBFRAME_HIGHT  24
#define SUBFRAME_SIZE   (SUBFRAME_WIDTH*SUBFRAME_HIGHT)
#define SUBFRAME_STEP   12
#define NUM_FRAMES      ((240/SUBFRAME_STEP)-1)

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

counter_status_t cs;

static ra_filter_t ra_filter;
httpd_handle_t pills_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

Eloquent::ML::Port::RandomForest classifier;
static volatile uint64_t pills_ctr = 0;

static ra_filter_t * ra_filter_init(ra_filter_t * filter, size_t sample_size){
    memset(filter, 0, sizeof(ra_filter_t));

    filter->values = (int *)malloc(sample_size * sizeof(int));
    if(!filter->values){
        return NULL;
    }
    memset(filter->values, 0, sample_size * sizeof(int));

    filter->size = sample_size;
    return filter;
}

/* moving average of frames per second */
static int ra_filter_run(ra_filter_t * filter, int value){
    if(!filter->values){
        return value;
    }
    filter->sum -= filter->values[filter->index];
    filter->values[filter->index] = value;
    filter->sum += filter->values[filter->index];
    filter->index++;
    filter->index = filter->index % filter->size;
    if (filter->count < filter->size) {
        filter->count++;
    }
    return filter->sum / filter->count;
}

static uint16_t grid_division(dl_matrix3du_t *image_matrix) {
    uint16_t si = 0;
    uint16_t pctr = 0;
    uint8_t subframe[SUBFRAME_SIZE] = {0,};

    for (uint16_t i = 0; i < image_matrix->h; i+=SUBFRAME_HIGHT) {
        for (uint16_t j = 0; j < image_matrix->w*3; j+=SUBFRAME_WIDTH*3) {
            for (uint16_t h = 0; h < SUBFRAME_HIGHT; h++) {
                for (uint16_t w = 0; w < SUBFRAME_WIDTH*3; w+=3) {
                    /*                                     intraframe                within frame     */
                    subframe[si++] = image_matrix->item[i*image_matrix->w*3+j + h*image_matrix->w*3+w];
                }
            }
            if (classifier.predict(subframe)) {
                pctr++;
                Serial.printf("found pill i = %d j = %d\n", i, j);
            }
            si = 0;
        }
    }

    return pctr;
}

static uint16_t one_detection_line(dl_matrix3du_t *image_matrix, uint16_t *detline) {
    uint16_t si = 0;
    uint16_t pctr = 0;
    uint16_t nframe = 0;

    uint8_t subframe[SUBFRAME_SIZE] = {0,};

    for (uint16_t i = 0; i < (image_matrix->w-SUBFRAME_WIDTH)*3; i+=SUBFRAME_STEP*3) {
        for (uint16_t h = 0; h < SUBFRAME_HIGHT; h++) {
            for (uint16_t w = 0; w < SUBFRAME_WIDTH*3; w+=3) {
                //                        intraframe         within frame     
                subframe[si++] = image_matrix->item[i + h*image_matrix->w*3+w];
            }
        }
        si = 0;

        nframe = i/(SUBFRAME_STEP*3);
        if (classifier.predict40(subframe)) {
            if (detline[nframe] == PILL_NO || detline[nframe] == PILL_DISAPPEARED) {
                detline[nframe] = PILL_DETECTED;
                Serial.printf("found pill f = %d\n", nframe);
            }
        } else {
            if (detline[nframe] == PILL_DETECTED) {
                detline[nframe] = PILL_DISAPPEARED;
                Serial.printf("disap pill f = %d\n", nframe);

                // get the first PILL_NO
                int16_t pstart = nframe;
                while (pstart >= 0 && detline[pstart] != PILL_NO) pstart--;
                // pstart = -1 [++ -> 0] or 
                // detline[pstart] = PILL_NO [++ -> PILL_DISAPPEARED or PILL_DETECTED]
                pstart++;
                while (pstart < NUM_FRAMES && detline[pstart] == PILL_DISAPPEARED) pstart++;
                
                // if all frames are PILL_DISAPPEARED then pstart is on PILL_NO or overflowed
                if (pstart == NUM_FRAMES || detline[pstart] == PILL_NO) {
                    pctr++;
                    // pstart = NUM_FRAMES [-- -> NUM_FRAMES-1] or 
                    // detline[pstart] = PILL_NO [-- -> PILL_DISAPPEARED]
                    pstart--;
                    Serial.printf("pill summed f = %d\n", pstart);
                    while (pstart >= 0 && detline[pstart] == PILL_DISAPPEARED) {
                        detline[pstart] = PILL_NO;
                        pstart--;
                    }
                }
            }
        }
    }

    return pctr;
}

static uint16_t two_detection_lines(dl_matrix3du_t *image_matrix, uint16_t detline[NUM_FRAMES][2]) {
    uint16_t si = 0;
    uint16_t pctr = 0;
    uint16_t nframe = 0;

    uint8_t subframe[SUBFRAME_SIZE] = {0,};

    for (uint16_t i = 0; i < (image_matrix->w-SUBFRAME_WIDTH)*3; i+=SUBFRAME_STEP*3) {
        for (uint16_t h = 0; h < SUBFRAME_HIGHT; h++) {
            for (uint16_t w = 0; w < SUBFRAME_WIDTH*3; w+=3) {
                //                        intraframe         within frame     
                subframe[si++] = image_matrix->item[i + h*image_matrix->w*3+w];
            }
        }
        si = 0;

        // First line detection
        nframe = i/(SUBFRAME_STEP*3);
        if (classifier.predict40(subframe)) {
            if (detline[nframe][0] == PILL_NO) {
                detline[nframe][0] = PILL_DETECTED;
                Serial.printf("line 1 found pill f = %d\n", nframe);
            }
        } else {
            if (detline[nframe][0] == PILL_DETECTED) {
                detline[nframe][0] = PILL_NO;
                detline[nframe][1] = PILL_EXPECTED;
                Serial.printf("line 1 disap pill f = %d\n", nframe);
            }
        }

        // second line detection (adjacent to the first one)
        for (uint16_t h = SUBFRAME_HIGHT; h < SUBFRAME_HIGHT*2; h++) {
            for (uint16_t w = 0; w < SUBFRAME_WIDTH*3; w+=3) {
                //                        intraframe         within frame     
                subframe[si++] = image_matrix->item[i + h*image_matrix->w*3+w];
            }
        }
        si = 0;

        if (classifier.predict40(subframe)) {
            if (detline[nframe][1] == PILL_NO || detline[nframe][1] == PILL_EXPECTED) {
                detline[nframe][1] = PILL_DETECTED;
                Serial.printf("line 2 found pill f = %d\n", nframe);
            }
        } else {
            if (detline[nframe][1] == PILL_DETECTED) {
                detline[nframe][1] = PILL_DISAPPEARED;
                Serial.printf("line 2 disap pill f = %d\n", nframe);

                // get the first PILL_NO
                int16_t pstart = nframe;
                while (pstart >= 0 && detline[pstart][1] != PILL_NO) pstart--;
                // pstart = -1 [++ -> 0] or 
                // detline[pstart] = PILL_NO [++ -> PILL_DISAPPEARED or PILL_DETECTED]
                pstart++;
                while (pstart < NUM_FRAMES && (detline[pstart][1] == PILL_DISAPPEARED || detline[pstart][1] == PILL_EXPECTED)) pstart++;
                
                // if all frames are PILL_DISAPPEARED then pstart is on PILL_NO or overflowed
                if (pstart == NUM_FRAMES || detline[pstart][1] == PILL_NO) {
                    pctr++;
                    // pstart = NUM_FRAMES [-- -> NUM_FRAMES-1] or 
                    // detline[pstart] = PILL_NO [-- -> PILL_DISAPPEARED]
                    pstart--;
                    Serial.printf("pill summed f = %d\n", pstart);
                    while (pstart >= 0 && (detline[pstart][1] == PILL_DISAPPEARED || detline[pstart][1] == PILL_EXPECTED)) {
                        detline[pstart][1] = PILL_NO;
                        pstart--;
                    }
                }
            }
        }
    }

    return pctr;
}

/* One Detection line method with 1 frame hysteresys */
static uint16_t one_detection_line_hys1(dl_matrix3du_t *image_matrix, uint16_t *detline) {
    uint16_t si = 0;
    uint16_t pctr = 0;
    uint16_t nframe = 0;

    uint8_t subframe[SUBFRAME_SIZE] = {0,};

    for (uint16_t i = 0; i < (image_matrix->w-SUBFRAME_WIDTH)*3; i+=SUBFRAME_STEP*3) {
        for (uint16_t h = 0; h < SUBFRAME_HIGHT; h++) {
            for (uint16_t w = 0; w < SUBFRAME_WIDTH*3; w+=3) {
                //                        intraframe         within frame     
                subframe[si++] = image_matrix->item[i + h*image_matrix->w*3+w];
            }
        }
        si = 0;

        nframe = i/(SUBFRAME_STEP*3);
        if (classifier.predict40(subframe)) {
            if (detline[nframe] == PILL_NO || detline[nframe] == PILL_DISAPPEARED) {
                detline[nframe] = PILL_DETECTED;
                Serial.printf("found pill f = %d\n", nframe);
            }
        } else {
            if (detline[nframe] == PILL_DETECTED) {
                detline[nframe] = PILL_DISAPPEARED;
                Serial.printf("disap pill f = %d\n", nframe);
            } else if (detline[nframe] == PILL_DISAPPEARED) {
                detline[nframe] = PILL_DISAPPEARED_HYS_1;
                // get the first PILL_NO
                int16_t pstart = nframe;
                while (pstart >= 0 && detline[pstart] != PILL_NO) pstart--;
                // pstart = -1 [++ -> 0] or 
                // detline[pstart] = PILL_NO [++ -> PILL_DISAPPEARED or PILL_DETECTED]
                pstart++;
                while (pstart < NUM_FRAMES && detline[pstart] == PILL_DISAPPEARED_HYS_1) pstart++;
                
                // if all frames are PILL_DISAPPEARED then pstart is on PILL_NO or overflowed
                if (pstart == NUM_FRAMES || detline[pstart] == PILL_NO) {
                    pctr++;
                    // pstart = NUM_FRAMES [-- -> NUM_FRAMES-1] or 
                    // detline[pstart] = PILL_NO [-- -> PILL_DISAPPEARED]
                    pstart--;
                    Serial.printf("pill summed f = %d\n", pstart);
                    while (pstart >= 0 && detline[pstart] == PILL_DISAPPEARED_HYS_1) {
                        detline[pstart] = PILL_NO;
                        pstart--;
                    }
                }
            }
        }
    }

    return pctr;
}

static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return httpd_resp_send(req, (const char *)index_html_gz, index_html_gz_len);
}

static esp_err_t pills_handler(httpd_req_t *req) {
    esp_err_t res = ESP_OK;
    char *ctr_str = (char *)malloc(sizeof(char)*10);

    sprintf(ctr_str, "%llu", pills_ctr);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    res = httpd_resp_send(req, (const char *)ctr_str, strlen(ctr_str));
    
    return res;
}

extern EventGroupHandle_t evGroup;

/* Note, currently shares ra_filter with /stream endpoint */
static esp_err_t counter_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];
    dl_matrix3du_t *image_matrix = NULL;

    uint16_t detline[NUM_FRAMES][2] = {0,};
    memset(detline, PILL_NO, NUM_FRAMES*2);
    // uint16_t detlines[NUM_FRAMES][2] = {0,};
    // memset(detlines, PILL_NO, NUM_FRAMES*2);

    int face_id = 0;
    bool detected = false;
    int64_t fr_start = 0;
    int64_t fr_face = 0;
    int64_t fr_recognize = 0;
    int64_t fr_encode = 0;
    int64_t fr_ready = 0;


    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK) {
        return res;
    }

    while (true) {
        detected = false;
        face_id = 0;
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        } else {
            fr_start = esp_timer_get_time();
            fr_ready = fr_start;
            fr_face = fr_start; 
            fr_encode = fr_start;
            fr_recognize = fr_start;
            if (fb->width > 400 || !cs.counter_enable) {
                if(fb->format != PIXFORMAT_JPEG){
                    bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                    esp_camera_fb_return(fb);
                    fb = NULL;
                    if(!jpeg_converted){
                        Serial.println("JPEG compression failed");
                        res = ESP_FAIL;
                    }
                } else {
                    _jpg_buf_len = fb->len;
                    _jpg_buf = fb->buf;
                }
            } else {
                image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);

                if (!image_matrix) {
                    Serial.println("dl_matrix3du_alloc failed");
                    res = ESP_FAIL;
                } else {
                    if(!fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item)){
                        Serial.println("fmt2rgb888 failed");
                        res = ESP_FAIL;
                    } else {
                        fr_ready = esp_timer_get_time();

                        if (cs.ca == CA_ONE_LINE_DETECTION_HYS_1) {
                            pills_ctr += one_detection_line_hys1(image_matrix, *detline);
                        } else if (cs.ca == CA_GRID_DIVISION) {
                            pills_ctr = grid_division(image_matrix);
                        } else if (cs.ca == CA_ONE_LINE_DETECTION) {
                            pills_ctr += one_detection_line(image_matrix, *detline);
                        } else if (cs.ca == CA_TWO_LINES_DETECTION) {
                            pills_ctr += two_detection_lines(image_matrix, detline);
                        } else {
                            // one that works best
                            pills_ctr += one_detection_line_hys1(image_matrix, *detline);
                        }
                        
                        fr_face = esp_timer_get_time();
                        fr_recognize = fr_face;
                        if (1) {
                            if(!fmt2jpg(image_matrix->item, image_matrix->w*image_matrix->h*3, image_matrix->w, image_matrix->h, PIXFORMAT_RGB888, 90, &_jpg_buf, &_jpg_buf_len)){
                                Serial.println("fmt2jpg failed");
                                res = ESP_FAIL;
                            }
                            // esp_camera_fb_return(fb);
                            // fb = NULL;
                        } else {
                            _jpg_buf = fb->buf;
                            _jpg_buf_len = fb->len;
                        }
                        fr_encode = esp_timer_get_time();
                    }
                    esp_camera_fb_return(fb);
                    fb = NULL;
                    dl_matrix3du_free(image_matrix);
                }
            }
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(fb){
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if(_jpg_buf){
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if(res != ESP_OK){
            break;
        }

        int64_t fr_end = esp_timer_get_time();
        int64_t ready_time = (fr_ready - fr_start)/1000;
        int64_t face_time = (fr_face - fr_ready)/1000;
        int64_t recognize_time = (fr_recognize - fr_face)/1000;
        int64_t encode_time = (fr_encode - fr_recognize)/1000;
        int64_t process_time = (fr_encode - fr_start)/1000;
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time);
        if (1) {
            Serial.printf("MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps), %u+%u+%u+%u=%u %s%d\n",
                (uint32_t)(_jpg_buf_len),
                (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time,
                avg_frame_time, 1000.0 / avg_frame_time,
                (uint32_t)ready_time, (uint32_t)face_time, (uint32_t)recognize_time, (uint32_t)encode_time, (uint32_t)process_time,
                (detected)?"DETECTED ":"", face_id
                 );
        }
    }

    Serial.println("Stream ended");
    last_frame = 0;
    return res;
}

static esp_err_t cmd_handler(httpd_req_t *req){
    char*  buf;
    size_t buf_len;
    char variable[32] = {0,};
    char value[32] = {0,};

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if(!buf){
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
                httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK) {
            } else {
                free(buf);
                httpd_resp_send_404(req);
                return ESP_FAIL;
            }
        } else {
            free(buf);
            httpd_resp_send_404(req);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    uint8_t res = 0;
    Serial.printf("cmd_handler: '%s' -> '%s'\n", variable, value);

    if(!strcmp(variable, "c")) cs.counter_enable = (uint8_t) atoi(value);
    else if(!strcmp(variable, "ca")) cs.ca = (counting_algorithm_t) atoi(value);
    else if(!strcmp(variable, "a")) cs.alarm_enable = (uint8_t) atoi(value);
    else if(!strcmp(variable, "ac")) cs.alarm_count = (uint64_t) atoll(value);
    else if(!strcmp(variable, "al")) strncpy(cs.alarm_link, value, ALARM_LINK_MAX_SIZE);
    else if(!strcmp(variable, "rc")) pills_ctr = 0; // reset counter
    else if(!strcmp(variable, "rd")) ESP.restart(); // reset board
    else {
        res = 1;
    }

    if (res){
        return httpd_resp_send_500(req);
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t status_handler(httpd_req_t *req){
    static char json_response[1024];

    sensor_t * s = esp_camera_sensor_get();
    char * p = json_response;
    *p++ = '{';

    p+=sprintf(p, "\"c\":%u,", cs.counter_enable); // Counter Enable
    p+=sprintf(p, "\"ca\":%u,", (uint8_t)cs.ca); // Counting Algorithm
    p+=sprintf(p, "\"a\":%d,", cs.alarm_enable); // Alarm Enable
    p+=sprintf(p, "\"ac\":%llu,", cs.alarm_count); // Alarm Count
    p+=sprintf(p, "\"al\":\"%s\"", cs.alarm_link); // Alarm Link
    *p++ = '}';
    *p++ = 0;

    Serial.printf("status_handler: %s\n", json_response);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json_response, strlen(json_response));
}

void startCameraServer(){
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t status_uri = {
        .uri       = "/status",
        .method    = HTTP_GET,
        .handler   = status_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t cmd_uri = {
        .uri       = "/control",
        .method    = HTTP_GET,
        .handler   = cmd_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t pills_uri = {
        .uri       = "/pills",
        .method    = HTTP_GET,
        .handler   = pills_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t counter_stream_uri = {
        .uri       = "/counter",
        .method    = HTTP_GET,
        .handler   = counter_handler,
        .user_ctx  = NULL
    };



    ra_filter_init(&ra_filter, 20);

    Serial.printf("Starting web server on port: '%d'", config.server_port);
    if (httpd_start(&pills_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(pills_httpd, &index_uri);
        httpd_register_uri_handler(pills_httpd, &pills_uri);
        httpd_register_uri_handler(pills_httpd, &cmd_uri);
        httpd_register_uri_handler(pills_httpd, &status_uri);
        
    }

    config.server_port++;
    config.ctrl_port++;
    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(camera_httpd, &counter_stream_uri);
    }
}