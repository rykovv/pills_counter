#include "ml_model/ml_model.h"

uint8_t classify_rf_d20(uint8_t * sample) {
    uint8_t votes[2] = { 0 };
    // tree #1
    if (sample[345] <= 195) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #2
    if (sample[347] <= 202) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #3
    if (sample[253] <= 203) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #4
    if (sample[352] <= 196) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #5
    if (sample[296] <= 204) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #6
    if (sample[272] <= 175) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #7
    if (sample[325] <= 200) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #8
    if (sample[273] <= 198) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #9
    if (sample[370] <= 201) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #10
    if (sample[278] <= 198) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #11
    if (sample[323] <= 192) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #12
    if (sample[251] <= 204) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #13
    if (sample[326] <= 200) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #14
    if (sample[251] <= 203) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #15
    if (sample[299] <= 201) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #16
    if (sample[252] <= 201) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #17
    if (sample[350] <= 199) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #18
    if (sample[256] <= 197) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #19
    if (sample[370] <= 176) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #20
    if (sample[253] <= 176) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // return argmasample of votes
    uint8_t classIdsample = 0;
    uint8_t masampleVotes = votes[0];

    for (uint8_t i = 1; i < 2; i++) {
        if (votes[i] > masampleVotes) {
            classIdsample = i;
            masampleVotes = votes[i];
        }
    }

    return classIdsample;
}

uint8_t classify_rf_d30 (uint8_t * sample) {
    uint8_t votes[2] = { 0 };
    // tree #1
    if (sample[346] <= 195) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #2
    if (sample[321] <= 192) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #3
    if (sample[348] <= 201) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #4
    if (sample[346] <= 170) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #5
    if (sample[254] <= 196) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #6
    if (sample[347] <= 201) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #7
    if (sample[278] <= 198) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #8
    if (sample[272] <= 202) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #9
    if (sample[370] <= 201) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #10
    if (sample[348] <= 199) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #11
    if (sample[297] <= 195) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #12
    if (sample[397] <= 199) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #13
    if (sample[227] <= 196) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #14
    if (sample[229] <= 196) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #15
    if (sample[325] <= 200) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #16
    if (sample[328] <= 197) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #17
    if (sample[204] <= 199) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #18
    if (sample[298] <= 202) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #19
    if (sample[255] <= 199) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #20
    if (sample[296] <= 201) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #21
    if (sample[202] <= 202) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #22
    if (sample[346] <= 195) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #23
    if (sample[249] <= 198) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #24
    if (sample[397] <= 197) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #25
    if (sample[300] <= 194) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #26
    if (sample[250] <= 197) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #27
    if (sample[328] <= 193) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #28
    if (sample[401] <= 196) {
        if (sample[207] <= 204) {
            votes[0] += 1;
        } else {
            votes[1] += 1;
        }
    } else {
        votes[1] += 1;
    }

    // tree #29
    if (sample[272] <= 189) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #30
    if (sample[396] <= 197) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // return argmasample of votes
    uint8_t classIdsample = 0;
    uint8_t masampleVotes = votes[0];

    for (uint8_t i = 1; i < 2; i++) {
        if (votes[i] > masampleVotes) {
            classIdsample = i;
            masampleVotes = votes[i];
        }
    }

    return classIdsample;
}

uint8_t classify_rf_d40 (uint8_t * sample) {
    uint8_t votes[2] = { 0 };
    // tree #1
    if (sample[295] <= 193) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #2
    if (sample[277] <= 202) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #3
    if (sample[373] <= 204) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #4
    if (sample[320] <= 199) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #5
    if (sample[225] <= 195) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #6
    if (sample[250] <= 197) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #7
    if (sample[249] <= 198) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #8
    if (sample[397] <= 197) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #9
    if (sample[228] <= 195) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #10
    if (sample[320] <= 196) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #11
    if (sample[296] <= 201) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #12
    if (sample[327] <= 200) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #13
    if (sample[367] <= 199) {
        if (sample[149] <= 203) {
            votes[0] += 1;
        } else {
            votes[1] += 1;
        }
    } else {
        votes[1] += 1;
    }

    // tree #14
    if (sample[353] <= 192) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #15
    if (sample[254] <= 196) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #16
    if (sample[224] <= 198) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #17
    if (sample[271] <= 198) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #18
    if (sample[303] <= 202) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #19
    if (sample[399] <= 201) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #20
    if (sample[377] <= 196) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #21
    if (sample[226] <= 200) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #22
    if (sample[225] <= 186) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #23
    if (sample[392] <= 197) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #24
    if (sample[370] <= 201) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #25
    if (sample[395] <= 198) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #26
    if (sample[327] <= 200) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #27
    if (sample[324] <= 204) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #28
    if (sample[347] <= 201) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #29
    if (sample[226] <= 175) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #30
    if (sample[229] <= 196) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #31
    if (sample[324] <= 204) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #32
    if (sample[303] <= 202) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #33
    if (sample[295] <= 193) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #34
    if (sample[344] <= 197) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #35
    if (sample[298] <= 204) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #36
    if (sample[202] <= 202) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #37
    if (sample[371] <= 199) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #38
    if (sample[248] <= 196) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #39
    if (sample[373] <= 196) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // tree #40
    if (sample[229] <= 196) {
        votes[0] += 1;
    } else {
        votes[1] += 1;
    }

    // return argmasample of votes
    uint8_t classIdsample = 0;
    uint8_t masampleVotes = votes[0];

    for (uint8_t i = 1; i < 2; i++) {
        if (votes[i] > masampleVotes) {
            classIdsample = i;
            masampleVotes = votes[i];
        }
    }

    return classIdsample;
}

uint8_t classify_svm (uint8_t *sample) {
    return 0;
}