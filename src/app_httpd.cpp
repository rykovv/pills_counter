#include "Arduino.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "dl_lib_matrix3d.h"

#include "pills_index.h"

#include "model.h"

typedef enum {
    PILL_NO = 0,
    PILL_DETECTED,
    PILL_DISAPPEARED,
    PILL_DISAPPEARED_HYS_1
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

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";


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

static size_t jpg_encode_stream(void * arg, size_t index, const void* data, size_t len){
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if(!index){
        j->len = 0;
    }
    if(httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK){
        return 0;
    }
    j->len += len;
    return len;
}

static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, (const char *)index_html, index_html_len);
}

static esp_err_t capture_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    int64_t fr_start = esp_timer_get_time();

    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.printf("Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");

    size_t fb_len = 0;
    if(fb->format == PIXFORMAT_JPEG){
        fb_len = fb->len;
        res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
    } else {
        jpg_chunking_t jchunk = {req, 0};
        res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk)?ESP_OK:ESP_FAIL;
        httpd_resp_send_chunk(req, NULL, 0);
        fb_len = jchunk.len;
    }
    Serial.printf("buffer size = %d\n", fb_len);
    esp_camera_fb_return(fb);
    int64_t fr_end = esp_timer_get_time();
    Serial.printf("JPG: %uB %ums", (uint32_t)(fb_len), (uint32_t)((fr_end - fr_start)/1000));
    return res;
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

/*
static esp_err_t stream_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];

    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    while(true){

        // xEventGroupWaitBits(evGroup, 1, pdFALSE, pdFALSE, portMAX_DELAY);

        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.printf("Camera capture failed");
            res = ESP_FAIL;
        } else {
            if(fb->format != PIXFORMAT_JPEG){
                bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                esp_camera_fb_return(fb);
                fb = NULL;
                if(!jpeg_converted){
                    Serial.printf("JPEG compression failed");
                    res = ESP_FAIL;
                }
            } else {
                _jpg_buf_len = fb->len;
                _jpg_buf = fb->buf;
            }
        }
        
        Serial.printf("buffer size = %d\n", _jpg_buf_len);

        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
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

        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time);
        Serial.printf("MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps)\n"
            ,(uint32_t)(_jpg_buf_len),
            (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time,
            avg_frame_time, 1000.0 / avg_frame_time
        );
    }

    last_frame = 0;
    return res;
}
*/

#define NUM_FRAMES  ((240/SUBFRAME_STEP)-1)
/* Note, currently shares ra_filter with /stream endpoint */
static esp_err_t counter_stream_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];
    dl_matrix3du_t *image_matrix = NULL;
    uint8_t subframe[SUBFRAME_SIZE] = {0,};
    uint16_t detline[NUM_FRAMES] = {0,};

    memset(detline, PILL_NO, NUM_FRAMES);

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
    if(res != ESP_OK){
        
        return res;
    }

    while(true){
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
            if(fb->width > 400){
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

                        /* I can work on that image (every third pixel) */

                        /* TODO: Test what is better: step every 3 pixels or make one channel matrix */
                        /* Line is 240*3.  */
                        uint16_t si = 0;
                        // uint32_t sid = 0;
                        // grid image division method
                        // for (uint16_t i = 0; i < image_matrix->h; i+=SUBFRAME_HIGHT) {
                        //     for (uint16_t j = 0; j < image_matrix->w*3; j+=SUBFRAME_WIDTH*3) {
                        //         for (uint16_t h = 0; h < SUBFRAME_HIGHT; h++) {
                        //             for (uint16_t w = 0; w < SUBFRAME_WIDTH*3; w+=3) {
                        //                 /*                                     intraframe                within frame     */
                        //                 subframe[si++] = image_matrix->item[i*image_matrix->w*3+j + h*image_matrix->w*3+w];
                        //                 sid += subframe[si-1];
                        //             }
                        //         }
                        //         if (classifier.predict(subframe)) {
                        //             pills_ctr++;
                        //             Serial.printf("found pill sid = %d i = %d j = %d\n", sid, i, j);
                        //         } else {
                        //             // Serial.printf("no pill found i = %d j = %d\n", i, j);
                        //         }
                        //         si = sid = 0;
                        //     }
                        // }
                        
                        uint16_t nframe = 0;
                        for (uint16_t i = 0; i < (image_matrix->w-SUBFRAME_WIDTH)*3; i+=SUBFRAME_STEP*3) {
                            for (uint16_t h = 0; h < SUBFRAME_HIGHT; h++) {
                                for (uint16_t w = 0; w < SUBFRAME_WIDTH*3; w+=3) {
                                    /*                        intraframe         within frame     */
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
                                        pills_ctr++;
                                        // pstart = NUM_FRAMES [-- -> NUM_FRAMES-1] or 
                                        // detline[pstart] = PILL_NO [-- -> PILL_DISAPPEARED]
                                        pstart--;
                                        while (pstart >= 0 && detline[pstart] == PILL_DISAPPEARED) {
                                            detline[pstart] = PILL_NO;
                                            pstart--;
                                        }
                                    }

                                    /*
                                    uint16_t fprev = PILL_NO;
                                    for (uint16_t i = pstart; i < NUM_FRAMES; i++) {
                                        if (fprev == PILL_NO && detline[i] == PILL_DISAPPEARED) {
                                            // pill starts
                                            pstart = i;
                                            fprev = PILL_DISAPPEARED;
                                        }
                                        if (fprev == PILL_DISAPPEARED && detline[i] == PILL_DETECTED) {
                                            // pill not closed
                                            pstart = 0;
                                            fprev = PILL_NO;
                                            Serial.printf("pill not closed f = %d\n", i-1);
                                        }
                                        // TODO: fix that case
                                        if (fprev == PILL_DISAPPEARED && detline[i] == PILL_NO) {
                                            // pill done
                                            pills_ctr++;
                                            for (uint16_t j = i-1; j >= pstart; j--) {
                                                detline[j] = PILL_NO;
                                            }
                                            pstart = 0;
                                            fprev = PILL_NO;
                                            Serial.printf("pill summed f = %d\n", i-1);
                                        }
                                    }
                                    */
                                }
                            }
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
        // pills_ctr = 0;
    }

    Serial.println("Stream ended");
    last_frame = 0;
    return res;
}

void startCameraServer(){
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t capture_uri = {
        .uri       = "/capture",
        .method    = HTTP_GET,
        .handler   = capture_handler,
        .user_ctx  = NULL
    };

    /*
    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };
    */

    httpd_uri_t counter_stream_uri = {
        .uri       = "/counter_stream",
        .method    = HTTP_GET,
        .handler   = counter_stream_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t pills_uri = {
        .uri       = "/pills",
        .method    = HTTP_GET,
        .handler   = pills_handler,
        .user_ctx  = NULL
    };

    ra_filter_init(&ra_filter, 20);

    Serial.printf("Starting web server on port: '%d'", config.server_port);
    if (httpd_start(&pills_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(pills_httpd, &index_uri);
        httpd_register_uri_handler(pills_httpd, &pills_uri);
        
    }

    config.server_port++;
    config.ctrl_port++;
    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        // httpd_register_uri_handler(camera_httpd, &capture_uri);
        // httpd_register_uri_handler(camera_httpd, &stream_uri);
        httpd_register_uri_handler(camera_httpd, &counter_stream_uri);
    }
}