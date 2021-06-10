/** @file   httpd_server.cpp
 *  @brief  httpd service initializer and endpoint handlers definitions.
 *
 *  There are several important features implemented in this file:
 *      1. HTTP interface hosting
 *      2. Device remote configuration
 *      3. MJPG streaming
 *      4. Providing endpoints to get counter value and device status
 *
 *  @author Vladislav Rykov
 *  @bug    No known bugs.
*/

#include "httpd_server/httpd_server.h"

#include "Arduino.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"

#include "index_html.h"

#include "ml_counter/ml_counter.h"
#include "dl_lib_matrix3d.h"


typedef struct {
        httpd_req_t *req;
        size_t len;
} jpg_chunking_t;


#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

device_t *_dev = NULL;

httpd_handle_t pills_httpd = NULL;
httpd_handle_t camera_httpd = NULL;


static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return httpd_resp_send(req, (const char *)index_html_gz, index_html_gz_len);
}

static esp_err_t pills_handler(httpd_req_t *req) {
    esp_err_t res = ESP_OK;
    char *ctr_str = (char *)malloc(sizeof(char)*10);

    sprintf(ctr_str, "%llu", _dev->status.counter_value);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    res = httpd_resp_send(req, (const char *)ctr_str, strlen(ctr_str));
    
    return res;
}

// extern EventGroupHandle_t evGroup;

static esp_err_t counter_handler (httpd_req_t *req) {
    esp_err_t res = ESP_OK;
    uint8_t *_jpg_buf = NULL;
    size_t _jpg_buf_len = 0;
    char *part_buf[64];

    _dev->httpd_monitored = 1;

    if (!_dev->stats.last_frame) {
        _dev->stats.last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK) {
        return res;
    }

    for (;;) {
        _dev->shared.fb = esp_camera_fb_get();
        if (!_dev->shared.fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
        } else {
            _dev->stats.fr_start = esp_timer_get_time();

            if (_dev->shared.fb->width > 400 || !_dev->status.counter_enable) {
                if (_dev->shared.fb->format != PIXFORMAT_JPEG) {
                    if(!frame2jpg(_dev->shared.fb, 80, &_jpg_buf, &_jpg_buf_len)){
                        ESP_LOGE(TAG, "JPEG compression failed");
                        res = ESP_FAIL;
                    } else {
                        esp_camera_fb_return(_dev->shared.fb);
                        _dev->shared.fb = NULL;
                    }
                } else {
                    _jpg_buf = _dev->shared.fb->buf;
                    _jpg_buf_len = _dev->shared.fb->len;
                }
            } else {
                _dev->shared.image_matrix = dl_matrix3du_alloc(1, _dev->shared.fb->width, _dev->shared.fb->height, 3);

                if (!_dev->shared.image_matrix) {
                    ESP_LOGE(TAG, "dl_matrix3du_alloc failed");
                    res = ESP_FAIL;
                } else {
                    if(!fmt2rgb888(_dev->shared.fb->buf, _dev->shared.fb->len, _dev->shared.fb->format, _dev->shared.image_matrix->item)){
                        ESP_LOGE(TAG, "fmt2rgb888 failed");
                        res = ESP_FAIL;
                    } else {
                        _dev->stats.fr_decode = esp_timer_get_time();

                        if (_dev->status.ca == CA_ONE_LINE_DETECTION_HYS_1) {
                            _dev->status.counter_value += one_detection_line_hys1(_dev->shared.image_matrix, *_dev->shared.detline);
                        } else if (_dev->status.ca == CA_GRID_DIVISION) {
                            _dev->status.counter_value = grid_division(_dev->shared.image_matrix);
                        } else if (_dev->status.ca == CA_ONE_LINE_DETECTION) {
                            _dev->status.counter_value += one_detection_line(_dev->shared.image_matrix, *_dev->shared.detline);
                        } else if (_dev->status.ca == CA_TWO_LINES_DETECTION) {
                            _dev->status.counter_value += two_detection_lines(_dev->shared.image_matrix, _dev->shared.detline);
                        } else {
                            // one that works best
                            _dev->status.counter_value += one_detection_line_hys1(_dev->shared.image_matrix, *_dev->shared.detline);
                        }
                        _dev->stats.fr_detection = esp_timer_get_time();
                        
                        if(!fmt2jpg(_dev->shared.image_matrix->item, _dev->shared.image_matrix->w*_dev->shared.image_matrix->h*3, _dev->shared.image_matrix->w, _dev->shared.image_matrix->h, PIXFORMAT_RGB888, 90, &_jpg_buf, &_jpg_buf_len)){
                            ESP_LOGE(TAG, "fmt2jpg failed");
                            res = ESP_FAIL;
                        }
                        _dev->stats.fr_encode = esp_timer_get_time();
                    }
                    esp_camera_fb_return(_dev->shared.fb);
                    _dev->shared.fb = NULL;
                    dl_matrix3du_free(_dev->shared.image_matrix);
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
        if(_dev->shared.fb){
            esp_camera_fb_return(_dev->shared.fb);
            _dev->shared.fb = NULL;
            _jpg_buf = NULL;
        } else if(_jpg_buf){
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if(res != ESP_OK){
            break;
        }

        int64_t fr_end = esp_timer_get_time();
        int64_t conv_rgb888_time = (_dev->stats.fr_decode - _dev->stats.fr_start)/1000;
        int64_t detection_time = (_dev->stats.fr_detection - _dev->stats.fr_decode)/1000;
        int64_t encode_time = (_dev->stats.fr_encode - _dev->stats.fr_detection)/1000;
        int64_t process_time = (_dev->stats.fr_encode - _dev->stats.fr_start)/1000;
        int64_t frame_time = fr_end - _dev->stats.last_frame;
        _dev->stats.last_frame = fr_end;
        frame_time /= 1000;
        uint32_t avg_frame_time = moving_avg_run(&_dev->stats.ma, frame_time);

        ESP_LOGD(TAG, "MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps), %u+%u+%u=%u\n",
            (uint32_t)(_jpg_buf_len),
            (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time,
            avg_frame_time, 1000.0 / avg_frame_time,
            (uint32_t)conv_rgb888_time, (uint32_t)detection_time, (uint32_t)encode_time, (uint32_t)process_time
        );
    }

    ESP_LOGI(TAG, "Stream ended");
    _dev->httpd_monitored = 0;
    
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

    if(!strcmp(variable, "c")) _dev->status.counter_enable = (uint8_t) atoi(value);
    else if(!strcmp(variable, "ca")) _dev->status.ca = (counting_algorithm_t) atoi(value);
    else if(!strcmp(variable, "a")) _dev->status.alarm_enable = (uint8_t) atoi(value);
    else if(!strcmp(variable, "ac")) _dev->status.alarm_count = (uint64_t) atoll(value);
    else if(!strcmp(variable, "al")) strncpy(_dev->status.alarm_link, value, ALARM_LINK_MAX_SIZE);
    else if(!strcmp(variable, "rc")) _dev->status.counter_value = 0; // reset counter
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

    p+=sprintf(p, "\"c\":%u,", _dev->status.counter_enable); // Counter Enable
    p+=sprintf(p, "\"ca\":%u,", (uint8_t)_dev->status.ca); // Counting Algorithm
    p+=sprintf(p, "\"a\":%d,", _dev->status.alarm_enable); // Alarm Enable
    p+=sprintf(p, "\"ac\":%llu,", _dev->status.alarm_count); // Alarm Count
    p+=sprintf(p, "\"al\":\"%s\"", _dev->status.alarm_link); // Alarm Link
    *p++ = '}';
    *p++ = 0;
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json_response, strlen(json_response));
}

void httpd_server_init(device_t *ndev) {
    _dev = ndev;

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

    ESP_LOGD(TAG, "Starting web server on port: '%d'", config.server_port);
    if (httpd_start(&pills_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(pills_httpd, &index_uri);
        httpd_register_uri_handler(pills_httpd, &pills_uri);
        httpd_register_uri_handler(pills_httpd, &cmd_uri);
        httpd_register_uri_handler(pills_httpd, &status_uri);
    }

    // config.stack_size = 10000; Use for SVM
    config.server_port++;
    config.ctrl_port++;
    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(camera_httpd, &counter_stream_uri);
    }
}