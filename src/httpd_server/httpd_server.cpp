#include "httpd_server/httpd_server.h"

#include "Arduino.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"

#include "index_html.h"

#include "ml_counter/ml_counter.h"
#include "dl_lib_matrix3d.h"

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


#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

counter_status_t *_cs = NULL;
volatile uint8_t * volatile _fb_free = NULL;

static ra_filter_t ra_filter;
httpd_handle_t pills_httpd = NULL;
httpd_handle_t camera_httpd = NULL;


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

static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return httpd_resp_send(req, (const char *)index_html_gz, index_html_gz_len);
}

static esp_err_t pills_handler(httpd_req_t *req) {
    esp_err_t res = ESP_OK;
    char *ctr_str = (char *)malloc(sizeof(char)*10);

    sprintf(ctr_str, "%llu", _cs->pills_ctr);
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

    int64_t fr_start = 0;
    int64_t fr_face = 0;
    int64_t fr_recognize = 0;
    int64_t fr_encode = 0;
    int64_t fr_ready = 0;


    // memset(detline, 0, NUM_FRAMES*2);
    // memset(detline, PILL_NO, NUM_FRAMES*2);
    uint16_t detline[NUM_FRAMES][2];

    ml_counter_init(detline);
    set_ml_model(classify_rf_d40);

    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK) {
        return res;
    }

    for (;;) {
        // wait for fb to be taken
        while (!(*_fb_free));
        *_fb_free = 0;
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGD(TAG, "Camera capture failed");
            res = ESP_FAIL;
        } else {
            fr_start = esp_timer_get_time();
            fr_ready = fr_start;
            fr_face = fr_start; 
            fr_encode = fr_start;
            fr_recognize = fr_start;
            if(fb->format != PIXFORMAT_JPEG){
                bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                esp_camera_fb_return(fb);
                *_fb_free = 0;
                fb = NULL;
                if(!jpeg_converted){
                    ESP_LOGD(TAG, "JPEG compression failed");
                    res = ESP_FAIL;
                }
            } else {
                // copy jpg and release fb
                _jpg_buf_len = fb->len;
                _jpg_buf = fb->buf;
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
            *_fb_free = 0;
            fb = NULL;
            _jpg_buf = NULL;
        } else if(_jpg_buf) {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }

        if (res != ESP_OK) {
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
            ESP_LOGD(TAG, "MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps), %u+%u+%u+%u=%u %s%d\n",
                (uint32_t)(_jpg_buf_len),
                (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time,
                avg_frame_time, 1000.0 / avg_frame_time,
                (uint32_t)ready_time, (uint32_t)face_time, (uint32_t)recognize_time, (uint32_t)encode_time, (uint32_t)process_time,
                (detected)?"DETECTED ":"", face_id
            );
        }
    }

    ESP_LOGD(TAG, "Stream ended");
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

    ESP_LOGD(TAG, "cmd_handler: '%s' -> '%s'\n", variable, value);

    if(!strcmp(variable, "c")) _cs->counter_enable = (uint8_t) atoi(value);
    else if(!strcmp(variable, "ca")) _cs->ca = (counting_algorithm_t) atoi(value);
    else if(!strcmp(variable, "a")) _cs->alarm_enable = (uint8_t) atoi(value);
    else if(!strcmp(variable, "ac")) _cs->alarm_count = (uint64_t) atoll(value);
    else if(!strcmp(variable, "al")) strncpy(_cs->alarm_link, value, ALARM_LINK_MAX_SIZE);
    else if(!strcmp(variable, "rc")) _cs->pills_ctr = 0; // reset counter
    else if(!strcmp(variable, "rd")) esp_restart(); // reset board
    else {
        return httpd_resp_send_500(req);
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t status_handler(httpd_req_t *req){
    static char json_response[1024];

    char * p = json_response;
    *p++ = '{';

    p+=sprintf(p, "\"c\":%u,", _cs->counter_enable); // Counter Enable
    p+=sprintf(p, "\"ca\":%u,", (uint8_t)_cs->ca); // Counting Algorithm
    p+=sprintf(p, "\"a\":%d,", _cs->alarm_enable); // Alarm Enable
    p+=sprintf(p, "\"ac\":%llu,", _cs->alarm_count); // Alarm Count
    p+=sprintf(p, "\"al\":\"%s\"", _cs->alarm_link); // Alarm Link
    *p++ = '}';
    *p++ = 0;

    ESP_LOGD(TAG, "status_handler: %s\n", json_response);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json_response, strlen(json_response));
}

void httpd_server_init(counter_status_t *n_cs, volatile uint8_t *n_fb_free) {
    _cs = n_cs;
    _fb_free = n_fb_free;
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

    ESP_LOGD(TAG, "Starting web server on port: '%d'", config.server_port);
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