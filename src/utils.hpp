#pragma once

#include <exception>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <bitset>

#include <NTL/ZZ.h>
#include <cymric.h>

// To allow easy swapping out of types

typedef NTL::ZZ BigInt;
typedef std::vector<BigInt> BIVector;
typedef std::vector<BigInt> BIMatrix;
typedef std::vector<bool> BitVector;
typedef std::vector<bool> BitMatrix;

extern std::default_random_engine generator;
extern cymric_rng rng;

void utils_init();
void rand_init();

std::ostream& operator<<(std::ostream&, const std::vector<int> &);
std::ostream& operator<<(std::ostream&, const std::vector<BigInt> &);
std::ostream& operator<<(std::ostream&, const BitMatrix &);

class ex: public std::exception {
public:
    std::string value;
    ex(std::string str);
    ex();
    const char* what() const throw();
};
