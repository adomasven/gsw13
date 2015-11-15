/* A discrete gaussian sampler based on BLISS.
 */

#include <cmath>
#include "gaussSampler.hpp"


// binary search on a list

static inline int binsearch(uint64_t x, const uint64_t l[], int n, int st)
{
    int a, b;

    a = 0;
    while (st > 0) {
        b = a + st;
        if (b < n && x >= l[b])
            a = b;
        st >>= 1;
    }
    return a;
}


// Build CDF's for given sigma

GaussSampler::GaussSampler(long double sigma)
{
    int i;
    long double s, d, e;

    // 2/sqrt(2*Pi)  * (1 << 64) / sigma
    d = 0.7978845608028653558798L * (18446744073709551616.0L) / sigma;

    e = -0.5L / (sigma * sigma);
    s = 0.5L * d;
    cdf[0] = 0;
    for (i = 1; i < GAUSS_CDF_SIZE - 1; i++) {
        cdf[i] = s;
        if (cdf[i] == 0)        // overflow
            break;
        s += d * expl(e * ((long double) (i*i)));
    }
    for (; i < GAUSS_CDF_SIZE; i++) {
        cdf[i] = 0xFFFFFFFFFFFFFFFF;
    }
}

// sample from the distribution with binary search

int32_t GaussSampler::sample()
{
    int a;
    uint64_t x;

    cymric_random(&rng, &x, 8);
    a = binsearch(x, cdf, GAUSS_CDF_SIZE, GAUSS_CDF_STEP);

    return a;
}


