#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include "utils.hpp"

std::default_random_engine generator;
cymric_rng rng;

std::ostream& operator<<(std::ostream &o, const std::vector<int> &container){
    std::copy(container.begin(), container.end(), 
            std::ostream_iterator<int>(o, " "));
    return o;
}

std::ostream& operator<<(std::ostream &o, const std::vector<BigInt> &container){
    std::copy(container.begin(), container.end(), 
            std::ostream_iterator<BigInt>(o, " "));
    return o;
}

std::ostream& operator<<(std::ostream &o, const BitMatrix &container){
    std::copy(container.begin(), container.end(), 
            std::ostream_iterator<bool>(o, ""));
    return o;
}

void utils_init() {
    rand_init();
}

void rand_init() {
    char rand_buff[17];
    rand_buff[16] = 0;
    if (cymric_init()) {
        throw ex("cymric: ABI compatibility failure");
    }
    if (cymric_seed(&rng, 0, 0)) {
        throw ex("cymric: Seed failed");
    }
    cymric_random(&rng, rand_buff, 16);
    NTL::ZZ rand;
    std::string rand_string = rand_buff;
    std::stringstream rand_stream (rand_string);
    rand_stream >> rand;
    NTL::SetSeed(rand);
}

ex::ex() {
    ex("An unknown exception occured");
}

ex::ex(std::string str) {
    value = str;
}

const char* ex::what() const throw(){
    return value.c_str();
}
