#pragma once

#include "utils.hpp"
#include "gaussSampler.hpp"

#define sigma 3.8
#define sigma6 (int)(sigma*6)

class GSW {

public: 
    BigInt quotient; // q/sigma6 > 8(N + 1)^L
    unsigned int n, n_1; // n >= log(q/sigma)(k+110)/7.2
    unsigned int m; // m = O(n log q)
    unsigned int l; // l = floor(log q) + 1
    unsigned int N; // N = (n + 1) * l

    GaussSampler *gaussSampler;
    
    GSW();
    GSW(const int, const int);
    ~GSW();

    BIVector secret_key_gen() const; //sk = Z(n+1)_q
    BIMatrix public_key_gen(const BIVector& secret_key) const; //pk = Z(m, n+1)_q

    // C = flatten(message * identity + BitDecomp(R * A))
    BitMatrix encrypt(const BIMatrix& public_key, const BigInt& message) const;

    BigInt decrypt(const BIVector& private_key, const BitMatrix& cyphertext) const;
    bool decrypt_bit(const BIVector& private_key, const BitMatrix& cyphertext) const;

    // Homomorphic operations
    BitMatrix nand(const BitMatrix&, const BitMatrix&) const;


    // utility functions
    BIVector powers_of_2(const BIVector&) const ;

    BitVector bit_decomp(const BIVector&) const ;

    BIVector inverse_bit_decomp(const BitVector&) const ;
    BIVector inverse_bit_decomp(const BIVector&) const ;

    BitVector flatten(const BitVector&) const ;
    BitVector flatten(const BIVector&) const ;

};

