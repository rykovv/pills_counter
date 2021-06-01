#ifndef DEVICE_H
#define DEVICE_H

#include "Arduino.h"
#include "esp_camera.h"
#include "ml_counter/ml_counter.h"

// #define ENABLE_STATS

#define ALARM_LINK_MAX_SIZE     30
#define MOVING_AVG_SAMPLES      20

// should match with the html 
typedef enum {
    CA_GRID_DIVISION = 0,
    CA_ONE_LINE_DETECTION,
    CA_ONE_LINE_DETECTION_HYS_1,
    CA_TWO_LINES_DETECTION
} counting_algorithm_t;

/* Struct for storing device status */
typedef struct {
    uint8_t counter_enable = 0;
    counting_algorithm_t ca = CA_ONE_LINE_DETECTION_HYS_1;
    uint8_t alarm_enable = 0;
    uint64_t alarm_count = 0;
    char alarm_link[ALARM_LINK_MAX_SIZE];
    uint64_t counter_value = 0;
} device_status_t;

/* For moving average for image processing stats */
typedef struct {
        size_t size; //number of values used for filtering
        size_t index; //current value index
        size_t count; //value count
        int sum;
        int * values; //array to be filled with values
} moving_avg_t;

/* general structure for storing device processing statistics */
typedef struct {
    int64_t fr_start = 0;
    int64_t fr_detection = 0;
    int64_t fr_conv_rgb888 = 0;
    int64_t fr_encode = 0;
    int64_t last_frame = 0;
    moving_avg_t ma;
} device_stats_t;

typedef struct {
    camera_fb_t *fb = NULL;                 //< Frame Buffer poiner
    dl_matrix3du_t *image_matrix = NULL;    //< Image matrix pointer
    uint16_t detline[NUM_FRAMES][2];        //< Detection line vector
} shared_data_t;

typedef struct {
    device_status_t status;
    device_stats_t stats;
    shared_data_t shared;
    volatile uint8_t httpd_monitored = 0;
} device_t;

#ifdef __cplusplus
extern "C" {
#endif

moving_avg_t * moving_avg_init(moving_avg_t *ma, size_t sample_size);
int moving_avg_run(moving_avg_t *ma, int value);


void counter_init (void);
void counter_count ();

#ifdef __cplusplus
}
#endif

#endif // DEVICE_H