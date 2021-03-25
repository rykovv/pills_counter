#include "Arduino.h"

#pragma once
#include <cstdarg>
namespace Eloquent {
    namespace ML {
        namespace Port {
            class RandomForest {
                public:
                    /**
                    * Predict class for features vector
                    */
                    int predict(const float *x) {
                        uint8_t votes[2] = { 0 };
                        // tree #1
                        if (x[2825] <= 206.5) {
                            if (x[938] <= 68.0) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #2
                        if (x[2833] <= 207.0) {
                            if (x[132] <= 88.0) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #3
                        if (x[2171] <= 155.0) {
                            votes[0] += 1;
                        }

                        else {
                            if (x[91] <= 177.0) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        // tree #4
                        if (x[2763] <= 204.5) {
                            if (x[2205] <= 76.5) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #5
                        if (x[2826] <= 205.5) {
                            if (x[1047] <= 79.5) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #6
                        if (x[2507] <= 163.5) {
                            votes[0] += 1;
                        }

                        else {
                            if (x[3443] <= 196.5) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        // tree #7
                        if (x[2705] <= 207.5) {
                            if (x[3839] <= 95.5) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #8
                        if (x[2763] <= 204.5) {
                            if (x[208] <= 82.5) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #9
                        if (x[2959] <= 206.5) {
                            if (x[929] <= 73.0) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #10
                        if (x[2892] <= 208.0) {
                            if (x[935] <= 67.5) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #11
                        if (x[2894] <= 204.5) {
                            if (x[2218] <= 63.5) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #12
                        if (x[2820] <= 210.0) {
                            if (x[855] <= 94.0) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #13
                        if (x[924] <= 99.0) {
                            votes[1] += 1;
                        }

                        else {
                            if (x[2968] <= 221.5) {
                                votes[0] += 1;
                            }

                            else {
                                votes[1] += 1;
                            }
                        }

                        // tree #14
                        if (x[2445] <= 204.5) {
                            if (x[517] <= 68.5) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #15
                        if (x[2956] <= 210.0) {
                            votes[0] += 1;
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #16
                        if (x[2628] <= 209.0) {
                            if (x[4004] <= 83.5) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #17
                        if (x[84] <= 137.5) {
                            if (x[3215] <= 110.0) {
                                votes[0] += 1;
                            }

                            else {
                                votes[1] += 1;
                            }
                        }

                        else {
                            votes[0] += 1;
                        }

                        // tree #18
                        if (x[2561] <= 207.5) {
                            if (x[3928] <= 86.5) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #19
                        if (x[147] <= 148.5) {
                            if (x[2759] <= 141.0) {
                                votes[0] += 1;
                            }

                            else {
                                votes[1] += 1;
                            }
                        }

                        else {
                            votes[0] += 1;
                        }

                        // tree #20
                        if (x[2699] <= 205.5) {
                            if (x[4194] <= 91.5) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // return argmax of votes
                        uint8_t classIdx = 0;
                        float maxVotes = votes[0];

                        for (uint8_t i = 1; i < 2; i++) {
                            if (votes[i] > maxVotes) {
                                classIdx = i;
                                maxVotes = votes[i];
                            }
                        }

                        return classIdx;
                    }

                    /**
                    * Predict readable class name
                    */
                    const char* predictLabel(const float *x) {
                        return idxToLabel(predict(x));
                    }

                    /**
                    * Convert class idx to readable name
                    */
                    const char* idxToLabel(uint8_t classIdx) {
                        switch (classIdx) {
                            case 0:
                            return "none";
                            case 1:
                            return "pill";
                            default:
                            return "Houston we have a problem";
                        }
                    }

                protected:
            };
        }
    }
}