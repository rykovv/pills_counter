#ifndef ML_COUNTER_H
#define ML_COUNTER_H

#include "ml_model/ml_model.h"

#include "Arduino.h"
#include "dl_lib_matrix3d.h"

#define FRAME_WIDTH     240
#define SUBFRAME_WIDTH  24
#define SUBFRAME_HIGHT  24
#define SUBFRAME_STEP   12
#define SUBFRAME_SIZE   (SUBFRAME_WIDTH*SUBFRAME_HIGHT)
#define NUM_SUBFRAMES   ((FRAME_WIDTH/SUBFRAME_STEP)-(SUBFRAME_WIDTH/SUBFRAME_STEP-1))

/* Necessary for moving correctly the subframe */
_Static_assert(!(SUBFRAME_WIDTH % SUBFRAME_STEP), "Inconsistent subframe step.");

#ifdef __cplusplus
extern "C" {
#endif

/* Init detection line vector to right values */
void ml_counter_init(uint16_t detline[NUM_SUBFRAMES][2]);

/* Must call this function before using the library */
void set_ml_model(uint8_t (*classifier)(uint8_t *));

/* Grid detection method. Does not support accumulative counting. */
uint16_t grid_division(dl_matrix3du_t *image_matrix);

/* One Detection line detection method. */
uint16_t one_detection_line(dl_matrix3du_t *image_matrix, uint16_t *detline);

/* Two lines detection requires additional calibration which is a function of belt speed. */
uint16_t two_detection_lines(dl_matrix3du_t *image_matrix, uint16_t detline[NUM_SUBFRAMES][2]);

/* One Detection line method with 1 frame hysteresys */
uint16_t one_detection_line_hys1(dl_matrix3du_t *image_matrix, uint16_t *detline);

#ifdef __cplusplus
}
#endif

#endif