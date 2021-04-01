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
                    int predict(uint8_t *x) {
                        uint8_t votes[2] = { 0 };      
                        // tree #1
                        if (x[343] <= 205) {
                            if (x[10] <= 84) {       
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
                        if (x[372] <= 167) {
                            votes[0] += 1;
                        }

                        else {
                            if (x[167] <= 178) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        // tree #3
                        if (x[375] <= 206) {
                            if (x[458] <= 95) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #4
                        if (x[396] <= 206) {
                            if (x[47] <= 70) {
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
                        if (x[351] <= 208) {
                            if (x[335] <= 70) {
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
                        if (x[349] <= 210) {
                            if (x[154] <= 71) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #7
                        if (x[393] <= 213) {
                            if (x[117] <= 101) {
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
                        if (x[397] <= 220) {
                            if (x[54] <= 79) {
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
                        if (x[397] <= 211) {
                            if (x[52] <= 73) {
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
                        if (x[76] <= 101) {
                            votes[1] += 1;
                        }

                        else {
                            if (x[346] <= 221) {
                                votes[0] += 1;
                            }

                            else {
                                votes[1] += 1;
                            }
                        }

                        // tree #11
                        if (x[398] <= 212) {
                            if (x[121] <= 87) {
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
                        if (x[367] <= 205) {
                            if (x[204] <= 205) {
                                if (x[92] <= 95) {
                                    votes[1] += 1;
                                }

                                else {
                                    votes[0] += 1;
                                }
                            }

                            else {
                                votes[1] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #13
                        if (x[125] <= 102) {
                            votes[1] += 1;
                        }

                        else {
                            if (x[206] <= 218) {
                                votes[0] += 1;
                            }

                            else {
                                votes[1] += 1;
                            }
                        }

                        // tree #14
                        if (x[322] <= 160) {
                            votes[0] += 1;
                        }

                        else {
                            if (x[550] <= 195) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        // tree #15
                        if (x[397] <= 211) {
                            if (x[535] <= 92) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #16
                        if (x[396] <= 206) {
                            if (x[507] <= 91) {
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
                        if (x[324] <= 207) {
                            if (x[101] <= 88) {
                                votes[1] += 1;
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #18
                        if (x[345] <= 205) {
                            votes[0] += 1;
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #19
                        if (x[96] <= 149) {
                            if (x[306] <= 115) {
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
                        if (x[325] <= 210) {
                            if (x[544] <= 94) {
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
                    const char* predictLabel(uint8_t *x) {
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