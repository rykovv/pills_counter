#ifndef DEVICE_H
#define DEVICE_H

#include "Arduino.h"
#include "esp_camera.h"
#include "ml_counter/ml_counter.h"

#define ALARM_LINK_MAX_SIZE     30      ///< Alarm HTTP link max length
#define MOVING_AVG_SAMPLES      20      ///< Moving average window size

/**
 * @brief Defines counting algorithms
 * 
 * @note Should match with the .html 
*/ 
typedef enum {
    CA_GRID_DIVISION = 0,           ///< Grid division.
    CA_ONE_LINE_DETECTION,          ///< One line detection
    CA_ONE_LINE_DETECTION_HYS_1,    ///< One line detection with 1-frame hysteresys
    CA_TWO_LINES_DETECTION          ///< Two lines detection
} counting_algorithm_t;

/**
 * @brief Device status struct. 
*/
typedef struct {
    counting_algorithm_t ca = 
        CA_ONE_LINE_DETECTION_HYS_1;        ///< Current counting algorithm
    uint8_t counter_enable = 0;             ///< Counter enable flag
    uint64_t counter_value = 0;             ///< Counter value
    uint8_t alarm_enable = 0;               ///< Alarm enables flag
    uint64_t alarm_count = 0;               ///< Counter value for triggering the alarm
    char alarm_link[ALARM_LINK_MAX_SIZE];   ///< HTTP link to send alarm JSON to
} device_status_t;

/**
 * @brief Moving average struct used for image processing stats 
*/
typedef struct {
        size_t size;    ///< Number of samples used for the average
        size_t index;   ///< Next sample index
        size_t count;   ///< Current number of samples
        int sum;        ///< Sum of values to compute average over
        int *values;    ///< Array of samples
} moving_avg_t;

/** 
 * @brief Struct for storing image processing stats.
 */
typedef struct {
    int64_t fr_start = 0;           ///< Start processing time
    int64_t fr_detection = 0;       ///< Time spent during image classification
    int64_t fr_decode = 0;          ///< JPG to RGB888 conversion time
    int64_t fr_encode = 0;          ///< RGB888 to JPG conversion time
    int64_t last_frame = 0;         ///< End of last frame time
    moving_avg_t ma;                ///< Moving average struct
} device_stats_t;

/** 
 * @brief Struct for storing shared data between counting task thread and 
 *          httpd monitor. Both perform classification.
 */
typedef struct {
    camera_fb_t *fb = NULL;                 //< Frame Buffer poiner
    dl_matrix3du_t *image_matrix = NULL;    //< Image matrix pointer
    uint16_t detline[NUM_SUBFRAMES][2];        //< Detection line vector
} shared_data_t;

/** 
 * @brief Device status and control struct.
 * 
 * @details It wraps 3 most important structs related to device
 *          status, processing statistics, shared data, and
 *          working mode.
 */
typedef struct {
    device_status_t status;                 ///< Device status
    device_stats_t stats;                   ///< Stats-related variables and struct
    shared_data_t shared;                   ///< Struct with shared data
    volatile uint8_t httpd_monitored = 0;   ///< Variable for determining the mode of operation: unattended or httpd monitored.
} device_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Moving average initialization function.
 * 
 * @details Initilizes and allocates memory for the moving average struct.
 * 
 * @param[inout] ma         Pointer to moving average struct.
 * @param[in] sample_size   Size of moving average window. 
 * 
 * @returns Pointer to initilized moving average struct.
*/
moving_avg_t * moving_avg_init(moving_avg_t *ma, const size_t sample_size);

/**
 * @brief Function for updating the moving average.
 * 
 * @details Cancels out the oldest sample, writes the most recent one,
 *          and updates the moving average value.
 * 
 * @param[in] ma        Pointer to moving average struct.
 * @param[in] value     Next sample. 
 * 
 * @returns The moving average value.
*/
int moving_avg_run(moving_avg_t *ma, const int value);

#ifdef __cplusplus
}
#endif

#endif // DEVICE_H