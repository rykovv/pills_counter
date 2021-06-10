/** @file   ml_model.h
 *  @brief  Functions prototypes for pills classification.
 *
 *  Two algorithms have been used so far:
 *      1. Support Vector Machine (SVM)
 *      2. Random Forest with forest depths of 
 *          (a) 20
 *          (b) 30 
 *          (c) 40
 *
 *  @author Vladislav Rykov
 *  @bug    No known bugs.
*/

#ifndef ML_MODEL_H
#define ML_MODEL_H

#include "Arduino.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Random Forest classifier with forest depth of 20.
 * 
 * @details ~0.16ms inference time; ~3ms per 19 frames
 * 
 * @param[in] sample Pointer to a sample array to be classified.
 * 
 * @returns 1 on a pill classified, otherwise 0.
*/
uint8_t classify_rf_d20 (const uint8_t *const sample);

/**
 * @brief Random Forest classifier with forest depth of 30.
 * 
 * @details ~0.2ms inference time; ~4ms per 19 frames  
 * 
 * @param[in] sample Pointer to a sample array to be classified.
 * 
 * @returns 1 on a pill classified, otherwise 0.
*/
uint8_t classify_rf_d30 (const uint8_t *const sample);

/**
 * @brief Random Forest classifier with forest depth of 40.
 * 
 * @details ~0.3ms inference time; ~5ms per 19 frames  
 * 
 * @param[in] sample Pointer to a sample array to be classified.
 * 
 * @returns 1 on a pill classified, otherwise 0.
*/
uint8_t classify_rf_d40 (const uint8_t *const sample);

/**
 * @brief Support Vector Machine (SVM) classifier.
 * 
 * @details ~52ms inference time; ~989ms per 19 frames. 
 * 
 * @note When use, increase httpd server & counting thread stack sizes to 10000.
 * 
 * @param[in] sample Pointer to a sample array to be classified.
 * 
 * @returns 1 on a pill classified, otherwise 0.
*/
uint8_t classify_svm (const uint8_t *const sample);

#ifdef __cplusplus
}
#endif

#endif