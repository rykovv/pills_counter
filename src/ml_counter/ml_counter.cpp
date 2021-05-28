#include "ml_counter/ml_counter.h"

typedef enum {
    PILL_NO = 0,
    PILL_DETECTED,
    PILL_DISAPPEARED,
    PILL_DISAPPEARED_HYS_1,
    PILL_EXPECTED
} pill_state_t;

uint8_t (*classifier)(uint8_t *);

void ml_counter_init(uint16_t detline[NUM_FRAMES][2]) {
    memset(detline, PILL_NO, NUM_FRAMES*2);
}

void set_ml_model(uint8_t (*nclassifier)(uint8_t *)) {
    classifier = nclassifier;
}

uint16_t grid_division(dl_matrix3du_t *image_matrix) {
    uint16_t si = 0;
    uint16_t pctr = 0;
    uint8_t subframe[SUBFRAME_SIZE] = {0,};

    for (uint16_t i = 0; i < image_matrix->h; i+=SUBFRAME_HIGHT) {
        for (uint16_t j = 0; j < image_matrix->w*3; j+=SUBFRAME_WIDTH*3) {
            for (uint16_t h = 0; h < SUBFRAME_HIGHT; h++) {
                for (uint16_t w = 0; w < SUBFRAME_WIDTH*3; w+=3) {
                    /*                                     intraframe                within frame     */
                    subframe[si++] = image_matrix->item[i*image_matrix->w*3+j + h*image_matrix->w*3+w];
                }
            }
            if (classifier(subframe)) {
                pctr++;
                ESP_LOGD(TAG, "found pill i = %d j = %d\n", i, j);
            }
            si = 0;
        }
    }

    return pctr;
}

uint16_t one_detection_line(dl_matrix3du_t *image_matrix, uint16_t *detline) {
    uint16_t si = 0;
    uint16_t pctr = 0;
    uint16_t nframe = 0;

    uint8_t subframe[SUBFRAME_SIZE] = {0,};

    for (uint16_t i = 0; i < (image_matrix->w-SUBFRAME_WIDTH)*3; i+=SUBFRAME_STEP*3) {
        for (uint16_t h = 0; h < SUBFRAME_HIGHT; h++) {
            for (uint16_t w = 0; w < SUBFRAME_WIDTH*3; w+=3) {
                //                        intraframe         within frame     
                subframe[si++] = image_matrix->item[i + h*image_matrix->w*3+w];
            }
        }
        si = 0;

        nframe = i/(SUBFRAME_STEP*3);
        if (classifier(subframe)) {
            if (detline[nframe] == PILL_NO || detline[nframe] == PILL_DISAPPEARED) {
                detline[nframe] = PILL_DETECTED;
                ESP_LOGD(TAG, "found pill f = %d\n", nframe);
            }
        } else {
            if (detline[nframe] == PILL_DETECTED) {
                detline[nframe] = PILL_DISAPPEARED;
                ESP_LOGD(TAG, "disap pill f = %d\n", nframe);

                // get the first PILL_NO
                int16_t pstart = nframe;
                while (pstart >= 0 && detline[pstart] != PILL_NO) pstart--;
                // pstart = -1 [++ -> 0] or 
                // detline[pstart] = PILL_NO [++ -> PILL_DISAPPEARED or PILL_DETECTED]
                pstart++;
                while (pstart < NUM_FRAMES && detline[pstart] == PILL_DISAPPEARED) pstart++;
                
                // if all frames are PILL_DISAPPEARED then pstart is on PILL_NO or overflowed
                if (pstart == NUM_FRAMES || detline[pstart] == PILL_NO) {
                    pctr++;
                    // pstart = NUM_FRAMES [-- -> NUM_FRAMES-1] or 
                    // detline[pstart] = PILL_NO [-- -> PILL_DISAPPEARED]
                    pstart--;
                    ESP_LOGD(TAG, "pill summed f = %d\n", pstart);
                    while (pstart >= 0 && detline[pstart] == PILL_DISAPPEARED) {
                        detline[pstart] = PILL_NO;
                        pstart--;
                    }
                }
            }
        }
    }

    return pctr;
}

uint16_t two_detection_lines(dl_matrix3du_t *image_matrix, uint16_t detline[NUM_FRAMES][2]) {
    uint16_t si = 0;
    uint16_t pctr = 0;
    uint16_t nframe = 0;

    uint8_t subframe[SUBFRAME_SIZE] = {0,};

    for (uint16_t i = 0; i < (image_matrix->w-SUBFRAME_WIDTH)*3; i+=SUBFRAME_STEP*3) {
        for (uint16_t h = 0; h < SUBFRAME_HIGHT; h++) {
            for (uint16_t w = 0; w < SUBFRAME_WIDTH*3; w+=3) {
                //                        intraframe         within frame     
                subframe[si++] = image_matrix->item[i + h*image_matrix->w*3+w];
            }
        }
        si = 0;

        // First line detection
        nframe = i/(SUBFRAME_STEP*3);
        if (classifier(subframe)) {
            if (detline[nframe][0] == PILL_NO) {
                detline[nframe][0] = PILL_DETECTED;
                ESP_LOGD(TAG, "line 1 found pill f = %d\n", nframe);
            }
        } else {
            if (detline[nframe][0] == PILL_DETECTED) {
                detline[nframe][0] = PILL_NO;
                detline[nframe][1] = PILL_EXPECTED;
                ESP_LOGD(TAG, "line 1 disap pill f = %d\n", nframe);
            }
        }

        // second line detection (adjacent to the first one)
        for (uint16_t h = SUBFRAME_HIGHT; h < SUBFRAME_HIGHT*2; h++) {
            for (uint16_t w = 0; w < SUBFRAME_WIDTH*3; w+=3) {
                //                        intraframe         within frame     
                subframe[si++] = image_matrix->item[i + h*image_matrix->w*3+w];
            }
        }
        si = 0;

        if (classifier(subframe)) {
            if (detline[nframe][1] == PILL_NO || detline[nframe][1] == PILL_EXPECTED) {
                detline[nframe][1] = PILL_DETECTED;
                ESP_LOGD(TAG, "line 2 found pill f = %d\n", nframe);
            }
        } else {
            if (detline[nframe][1] == PILL_DETECTED) {
                detline[nframe][1] = PILL_DISAPPEARED;
                ESP_LOGD(TAG, "line 2 disap pill f = %d\n", nframe);

                // get the first PILL_NO
                int16_t pstart = nframe;
                while (pstart >= 0 && detline[pstart][1] != PILL_NO) pstart--;
                // pstart = -1 [++ -> 0] or 
                // detline[pstart] = PILL_NO [++ -> PILL_DISAPPEARED or PILL_DETECTED]
                pstart++;
                while (pstart < NUM_FRAMES && (detline[pstart][1] == PILL_DISAPPEARED || detline[pstart][1] == PILL_EXPECTED)) pstart++;
                
                // if all frames are PILL_DISAPPEARED then pstart is on PILL_NO or overflowed
                if (pstart == NUM_FRAMES || detline[pstart][1] == PILL_NO) {
                    pctr++;
                    // pstart = NUM_FRAMES [-- -> NUM_FRAMES-1] or 
                    // detline[pstart] = PILL_NO [-- -> PILL_DISAPPEARED]
                    pstart--;
                    ESP_LOGD(TAG, "pill summed f = %d\n", pstart);
                    while (pstart >= 0 && (detline[pstart][1] == PILL_DISAPPEARED || detline[pstart][1] == PILL_EXPECTED)) {
                        detline[pstart][1] = PILL_NO;
                        pstart--;
                    }
                }
            }
        }
    }

    return pctr;
}

/* One Detection line method with 1 frame hysteresys */
uint16_t one_detection_line_hys1(dl_matrix3du_t *image_matrix, uint16_t *detline) {
    uint16_t si = 0;
    uint16_t pctr = 0;
    uint16_t nframe = 0;

    uint8_t subframe[SUBFRAME_SIZE] = {0,};

    for (uint16_t i = 0; i < (image_matrix->w-SUBFRAME_WIDTH)*3; i+=SUBFRAME_STEP*3) {
        for (uint16_t h = 0; h < SUBFRAME_HIGHT; h++) {
            for (uint16_t w = 0; w < SUBFRAME_WIDTH*3; w+=3) {
                //                        intraframe         within frame     
                subframe[si++] = image_matrix->item[i + h*image_matrix->w*3+w];
            }
        }
        si = 0;

        nframe = i/(SUBFRAME_STEP*3);
        if (classifier(subframe)) {
            if (detline[nframe] == PILL_NO || detline[nframe] == PILL_DISAPPEARED) {
                detline[nframe] = PILL_DETECTED;
                ESP_LOGD(TAG, "found pill f = %d\n", nframe);
            }
        } else {
            if (detline[nframe] == PILL_DETECTED) {
                detline[nframe] = PILL_DISAPPEARED;
                ESP_LOGD(TAG, "disap pill f = %d\n", nframe);
            } else if (detline[nframe] == PILL_DISAPPEARED) {
                detline[nframe] = PILL_DISAPPEARED_HYS_1;
                // get the first PILL_NO
                int16_t pstart = nframe;
                while (pstart >= 0 && detline[pstart] != PILL_NO) pstart--;
                // pstart = -1 [++ -> 0] or 
                // detline[pstart] = PILL_NO [++ -> PILL_DISAPPEARED or PILL_DETECTED]
                pstart++;
                while (pstart < NUM_FRAMES && detline[pstart] == PILL_DISAPPEARED_HYS_1) pstart++;
                
                // if all frames are PILL_DISAPPEARED then pstart is on PILL_NO or overflowed
                if (pstart == NUM_FRAMES || detline[pstart] == PILL_NO) {
                    pctr++;
                    // pstart = NUM_FRAMES [-- -> NUM_FRAMES-1] or 
                    // detline[pstart] = PILL_NO [-- -> PILL_DISAPPEARED]
                    pstart--;
                    ESP_LOGD(TAG, "pill summed f = %d\n", pstart);
                    while (pstart >= 0 && detline[pstart] == PILL_DISAPPEARED_HYS_1) {
                        detline[pstart] = PILL_NO;
                        pstart--;
                    }
                }
            }
        }
    }

    return pctr;
}