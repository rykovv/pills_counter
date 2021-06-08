#include "device/device.h"

moving_avg_t * moving_avg_init(moving_avg_t *ma, const size_t sample_size){
    memset(ma, 0, sizeof(moving_avg_t));

    ma->values = (int *)malloc(sample_size * sizeof(int));
    if (!ma->values) {
        return NULL;
    }
    memset(ma->values, 0, sample_size * sizeof(int));

    ma->size = sample_size;
    return ma;
}

int moving_avg_run (moving_avg_t *ma, const int value){
    if (!ma->values) {
        return value;
    }

    ma->sum -= ma->values[ma->index];
    ma->values[ma->index] = value;
    ma->sum += ma->values[ma->index];
    ma->index++;
    ma->index = ma->index % ma->size;

    if (ma->count < ma->size) {
        ma->count++;
    }

    return ma->sum / ma->count;
}