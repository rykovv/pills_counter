#ifndef ML_MODEL_H
#define ML_MODEL_H

#include "Arduino.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Random Forest classifier forest depth 20 */
uint8_t classify_rf_d20 (uint8_t *sample);

/* Random Forest classifier forest depth 30 */
uint8_t classify_rf_d30 (uint8_t *sample);

/* Random Forest classifier forest depth 40 */
uint8_t classify_rf_d40 (uint8_t *sample);

/* Support Vector Machine (SVM) classifier */
uint8_t classify_svm (uint8_t *sample);

#ifdef __cplusplus
}
#endif

#endif