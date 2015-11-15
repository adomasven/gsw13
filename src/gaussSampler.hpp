/* Implementation based on https://github.com/mjosaarinen/hilabliss
 */
#pragma once

#include "utils.hpp"

#define GAUSS_CDF_SIZE 0x1000
#define GAUSS_CDF_STEP 0x0800

class GaussSampler {
public:
    // table for binary search
    uint64_t cdf[GAUSS_CDF_SIZE];

    GaussSampler(long double sigma);
    int32_t sample();
};

