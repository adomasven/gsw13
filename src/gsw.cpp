#include <random>
#include <cmath>
#include <cassert>

#include <omp.h>

#include <NTL/ZZ.h>
#include <NTL/tools.h>

#include "gsw.hpp"

using namespace std;
using namespace NTL;

GSW::GSW() {
    GSW(80, 1);
}

GSW::GSW(const int kappa, const int L) {
    // Search for suitable parameters:
    // n >= log(q/sigma)(k+110)/7.2
    // q/sigma6 > 8(N + 1)^L
    BigInt lower_bound;
    n = (kappa+110)/7.2;
    quotient = 4;
    l = floor(log(quotient)/log(2)) + 1;
    N = (n + 1) * l;
    while (true) {
        power(lower_bound, N+1,L);
        lower_bound *= 8 * sigma6;
        if (quotient <= lower_bound) {
            NextPrime(quotient, lower_bound);
        } else {
            break;
        }
        n = log(quotient/ceil(sigma))*(kappa+110)/(7.2*log(2));
        l = floor(log(quotient)/log(2)) + 1;
        N = (n + 1) * l;
    }

    n_1 = n+1;
    m = ceil(n * log(quotient)/log(2));

    gaussSampler = new GaussSampler(sigma);

    omp_set_num_threads(4);
}

GSW::~GSW() {
    delete gaussSampler;
}


BIVector GSW::secret_key_gen() const {
    BIVector secret_key(n+1);
    // sample uniformly
    for (size_t i = 1; i < secret_key.size(); i++) {
        secret_key[i] = RandomBnd(quotient);
    }
    secret_key[0] = 1;
    return secret_key;
}

BIMatrix GSW::public_key_gen(const BIVector& sk) const {
    // recovering t from sk, defined as t = (-s_2,...,-s_n) in Z_q
    BIVector t(n);
    for (unsigned int i = 0; i < n; i++) {
        t[i] = quotient - sk[i+1];
    }

    // Uniformaly generated matrix (part of pk)
    BIMatrix B(m * n);
    for (unsigned int i = 0; i < m*n; i++)
        B[i] = RandomBnd(quotient);

    // First column of public key  b = B*t + e
    BIVector b(m);
    //BIVector e(m); // Error vector, optimized out
    BigInt temp;
    for (unsigned int i = 0; i < m; i++) { 
        for (unsigned int j = 0; j < n; j++) {
            MulMod(temp, B[i*n+j], t[j], quotient);
            AddMod(b[i], b[i], temp, quotient);
            //b[i] += B[i*n + j] * t[j];
            //b[i] = b[i] % quotient;
        }
        int bit = gaussSampler->sample() % sigma6;
        //e[i] = bit;
        b[i] += bit;
    }

    // Observe that pk * sk = e
    BIMatrix pk(m * n_1);
    for (unsigned int i = 0; i < m; i++) {
        pk[i*n_1] = b[i];
    }
    for (unsigned int i = 0; i < m*n; i++) {
        pk[1+i+(i/n)] = B[i];
    }

    // this is satisfied, the check proves it
#define DEBUG
#ifdef DEBUG
    BIVector e_1(m);
    for (unsigned int i = 0; i < m; i++) {
        e_1[i] = 0;
        for (unsigned int j = 0; j < n_1; j++) {
            e_1[i] += pk[i*n_1 + j] * sk[j];
            e_1[i] = e_1[i] % quotient;
        }
        //assert(e_1[i] == e[i]);
    }
#endif

    return pk;
}

BitMatrix GSW::encrypt(const BIMatrix& public_key, const BigInt& message) const {
    bernoulli_distribution bernoulli(0.5);

    BitMatrix R(N * m);
    for (size_t i = 0; i < R.size(); i++) {
        R[i] = bernoulli(generator);
    }
    BIMatrix RA(N * n_1);
    BigInt temp;
# pragma omp parallel for shared (R, public_key, RA) schedule(guided)
    for (unsigned int i = 0; i < N; i++) { 
        if (omp_get_thread_num() == 0)
            cerr << "Calc RA matrix " << i << " out of " << N << "\r";
        for (unsigned int j = 0; j < n_1; j++) {
            RA[i*n_1 + j] = 0;
            for (unsigned int k = 0; k < m; k++) {
                MulMod(temp, R[i*m + k], public_key[k*n_1 + j], quotient);
                AddMod(RA[i*n_1 + j], RA[i*n_1 + j], temp, quotient);
                //RA[i*n_1 + j] += R[i*m + k] * public_key[k*n_1 + j];
                //RA[i*n_1 + j] = RA[i*n_1 + j] % quotient;
            }
        }
    }
    cerr << endl;
    const BitMatrix RAbits = bit_decomp(RA);
    BIMatrix C(N * N);
# pragma omp parallel for shared (C, message) schedule(guided)
    for (unsigned int i = 0; i < N; i++) {
        if (omp_get_thread_num() == 0)
            cerr << "Calc ciphertext matrix " << i << " out of " << N << "\r";
        for (unsigned int j = 0; j < N; j++) {
            C[i*N + j] = RAbits[i*N + j];
            // message * identity 
            if (i == j) {
                C[i*N + j] += message;
            }
            //C[i*N + j] = C[i*N + j] % quotient;
        }
    }

    cerr << endl << "Now to flatten" << endl;

    return flatten(C);
}

BigInt GSW::decrypt(const BIVector& sk, const BitMatrix& C) const {
    BigInt m, it, fract;
    const auto v = powers_of_2(sk);
    BIVector powered_m_bits(l-1);
    for (unsigned int i = 0; i < l-1; i++) {
        powered_m_bits[i] = 0;
        for (unsigned int j = 0; j < N; j++) {
            powered_m_bits[i] += C[i*(l-1) + j] * v[j];
            powered_m_bits[i] = powered_m_bits[i] % quotient;
        }
    }
    m = 0;
    for (int i = l-2; i >= 0; i--) {
        it = (powered_m_bits[i] - pow(2, i)*m);
        fract = it % quotient/2;
        bool bit = fract >= quotient/4;

        m += bit << (l-2 - i);
    }

    return m;
}

bool GSW::decrypt_bit(const BIVector& sk, const BitMatrix& C) const {
    unsigned int i;
    const auto v = powers_of_2(sk);
    BigInt q_4, q_2; q_4 = quotient/4; q_2 = quotient/2;
    
    for(i = 0; i < l; i++) {
        if(v[i] > q_4 && v[i] <= q_2) break;
    }

    BigInt xi, temp;
    xi = 0;
    for (unsigned int j = 0; j < N; j++) {
        MulMod(temp, C[i*N + j], v[j], quotient);
        AddMod(xi, xi, temp, quotient);
        //xi += (C[i*N + j] * v[j]);
        //xi = xi % quotient;
    }

    return xi >= v[i]/2; 
}

BitMatrix GSW::nand(const BitMatrix& a, const BitMatrix& b) const {
    BIMatrix res(a.size());

    for (unsigned int i = 0; i < N; i++) {
        res[i*N + i] = 1;
    }

    BigInt temp;
# pragma omp parallel for shared (a, b, res) schedule(guided)
    for (unsigned int i = 0; i < N; i++) {
        if (omp_get_thread_num() == 0)
            cerr << "Performing a NAND, hold on " << i << " out of " << N << "\r";
        for (unsigned int j = 0; j < N; j++) {
            for (unsigned int k = 0; k < N; k++) {
                res[i*N + j] = res[i*N + j] + (a[i*N + k] & b[k*N + j]);
            }
            res[i*N + j] = res[i*N + j] % quotient;
        }
    }
    cerr << endl;

    return flatten(res);
}

//////////////////////////////////////////////
// Utility Functions
//////////////////////////////////////////////


BIVector GSW::powers_of_2(const BIVector& a) const {
    BIVector result(N);
# pragma omp parallel for shared (result, a) schedule(guided)
    for (unsigned int i = 0; i < n+1; i++) {
        for (unsigned int j = 0; j < l; j++) {
            MulMod(result[i*l + j], power2_ZZ(j), a[i], quotient);
            //result[i*l + j] = (BigInt) pow(2, j)*a[i] % quotient;
        }
    }
    
    return result;
}

BitVector GSW::bit_decomp(const BIVector& a) const {
    unsigned int num_rows = a.size() / n_1;
    BitVector result(n_1*l*num_rows);
    BigInt mask;
    mask = 1;
# pragma omp parallel for shared (result, a) schedule(guided)
    for (unsigned int j = 0; j < l; j++) {
        for (unsigned int k = 0; k < num_rows; k++) {
            for (unsigned int i = 0; i < n_1; i++) {
                result[k*l*n_1 + i*l + j] = (a[k*n_1 + i] & mask) > 0;
            }
        }
        mask <<= 1;
    }

    return result;
}

BIVector GSW::inverse_bit_decomp(const BitVector& a) const {
    BIVector result(n_1 * N);
    BigInt multiplier;
    multiplier = 1;

# pragma omp parallel for shared (result, a) schedule(guided)
    for (unsigned int j = 0; j < l; j++) {
        for (unsigned int row = 0; row < N; row++) {
            for (unsigned int i = 0; i < n_1; i++) {
                result[row * n_1 + i] += a[row*n_1*l + i*l + j] * multiplier;
            }
        }
        multiplier <<= 1;
    }

    return result; 
}

BIVector GSW::inverse_bit_decomp(const BIVector& a) const {
    BIVector result(n_1 * N);
    BigInt multiplier;
    multiplier = 1;

# pragma omp parallel for shared (result, a) schedule(guided)
    for (unsigned int j = 0; j < l; j++) {
        for (unsigned int row = 0; row < N; row++) {
            for (unsigned int i = 0; i < n_1; i++) {
                result[row * n_1 + i] += a[row*n_1*l + i*l + j] * multiplier;
            }
        }
        multiplier <<= 1;
    }

    return result; 
}

BitVector GSW::flatten(const BitVector& a) const {
    return bit_decomp(inverse_bit_decomp(a));
}

BitVector GSW::flatten(const BIVector& a) const {
    return bit_decomp(inverse_bit_decomp(a));
}
