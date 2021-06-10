/** @file   ml_counter.h
 *  @brief  Constants and function prototypes related to the pills 
 *          counting modes.
 *
 *  In the first place, the file defines crucial constants for pills counting 
 *  workflow. Depending on a selected counting mode, the image will be
 *  split into subframes of defined size. For grid detection all of them 
 *  must fit into one frame, for the rest of modes, all of them must fit
 *  into the frame width with no overflow. 
 *
 *  @author Vladislav Rykov
 *  @bug    No known bugs.
*/

#ifndef ML_COUNTER_H
#define ML_COUNTER_H

#include "ml_model/ml_model.h"

#include "Arduino.h"
#include "dl_lib_matrix3d.h"

#define FRAME_WIDTH     240     ///< Frame width. Taken as a main parameter since the detection line is placed along width dimension.
#define FRAME_HIGHT     240     ///< Frame hight.
#define SUBFRAME_WIDTH  24      ///< Subframe width.
#define SUBFRAME_HIGHT  24      ///< Subframe hight
#define SUBFRAME_STEP   12      ///< Subframe step. Offset to the next subframe
#define NUM_SUBFRAMES   ( (FRAME_WIDTH/SUBFRAME_STEP) -         \
                          (SUBFRAME_WIDTH/SUBFRAME_STEP-1) )        ///< Total number of subframes used for detection line(s) modes.
#define SUBFRAME_SIZE   (SUBFRAME_WIDTH*SUBFRAME_HIGHT)             ///< Subframe size

/* Necessary for fitting all subframes into one frame hight */
_Static_assert(!(FRAME_HIGHT % SUBFRAME_HIGHT), "Inconsistent subframe step for grid division.");
/* Necessary for fitting all subframes into one frame width */
_Static_assert(!(SUBFRAME_WIDTH % SUBFRAME_STEP), "Inconsistent subframe step.");
/* Subframe step should be less than the subframe width. */
_Static_assert((SUBFRAME_WIDTH / SUBFRAME_STEP), "Non-sense subframe step.");

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the counter.
 * 
 * @details Initializes detection line vector to right values and sets default classifier.
 * 
 * @param[inout] detline Detection line array.
 * 
*/
void ml_counter_init(uint16_t detline[NUM_SUBFRAMES][2]);

/**
 * @brief Sets classification model.
 * 
 * @param[in] classifer Pointer to a classification function.
 * 
*/
void set_ml_model(uint8_t (*classifier)(const uint8_t *const ));

/**
 * @brief Counts pills using grid detection counting method. 
 * 
 * @note Does not support accumulative counting.
 * 
 * @param[in] image_matrix Pointer to RGB888 image matrix.
 * 
 * @returns Number of found pills 
*/
uint16_t grid_division(const dl_matrix3du_t *const image_matrix);

/**
 * @brief Counts pills using One Detection line counting method. 
 * 
 * @param[in] image_matrix  Pointer to RGB888 image matrix.
 * @param[in] detline       Pointer to detection line array.
 * 
 * @returns Number of found pills 
*/
uint16_t one_detection_line(const dl_matrix3du_t *const image_matrix, uint16_t *detline);

/**
 * @brief Counts pills using Two Detection lines counting method. 
 * 
 * @note Requires additional calibration regaring the distance between two lines
 *          which is a function of production belt/conveyor speed.
 * 
 * @param[in] image_matrix  Pointer to RGB888 image matrix
 * @param[in] detline       Pointer to detection line array.
 * 
 * @returns Number of found pills 
*/
uint16_t two_detection_lines(const dl_matrix3du_t *const image_matrix, uint16_t detline[NUM_SUBFRAMES][2]);

/**
 * @brief Counts pills using One Detection line counting method with 1 frame hysteresis. 
 * 
 * @param[in] image_matrix  Pointer to RGB888 image matrix
 * @param[in] detline       Pointer to detection line array.
 * 
 * @returns Number of found pills 
*/
uint16_t one_detection_line_hys1(const dl_matrix3du_t *const image_matrix, uint16_t *detline);

#ifdef __cplusplus
}
#endif

#endif