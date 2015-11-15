#pragma once

#include "circuit.hpp"

#include "utils.hpp"
#include "gsw.hpp"

class CryptoCircuit : public CircuitBase<BitMatrix> {
public:
    CryptoCircuit();
    CryptoCircuit(std::string);
    CryptoCircuit(std::istream&);

    void reset();
    void eval(std::vector<BitMatrix>&, GSW&);
};
