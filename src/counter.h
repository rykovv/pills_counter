#ifndef COUNTER_H
#define COUNTER_H

#include "Arduino.h"

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
    uint64_t pills_ctr = 0;
} counter_status_t;

void counter_init (void);
void counter_count ();

#endif // COUNTER_H