#include <map>
#include <queue>

#include "cryptoCircuit.hpp"

using namespace std;

CryptoCircuit::CryptoCircuit() : CircuitBase() { }
CryptoCircuit::CryptoCircuit(string filename) : CircuitBase(filename) { }
CryptoCircuit::CryptoCircuit(istream& fp) : CircuitBase(fp) { }

void CryptoCircuit::reset() {
    queue<shared_ptr<Gate<BitMatrix> > > q;
    for (auto g : inputs) {
        g->id = -1; g->val.clear();
        for (auto out_g : g->outputs) {
            q.push(out_g);
        }
    }
    while (!q.empty()) {
        shared_ptr<Gate<BitMatrix> > g = q.front();
        q.pop();
        g->id = -1; g->val.clear();
        for (auto out_g : g->outputs) {
            q.push(out_g);
        }
    }
}

void CryptoCircuit::eval(vector<BitMatrix>& in, GSW& gsw) {
    reset();
    queue<shared_ptr<Gate<BitMatrix> > > q;
    for (uintmax_t i = 0; i < inputs.size(); i++) {
        inputs[i]->val = in[i];
        for (auto out_g : inputs[i]->outputs) {
            q.push(out_g);
        }
    }
    while (!q.empty()) {
        shared_ptr<Gate<BitMatrix> > g = q.front();
        q.pop();

        bool ready = true;
        for (auto in_g : g->inputs) {
            ready = ready && ! in_g->val.empty();
        }
        // Not all input values propagated yet
        if (!ready) {
            continue;
        }

        if (g->type != NAND) {
            throw runtime_error("CryptoCircuit can only NAND");
        } else {
            auto nand_result = gsw.nand(g->inputs[0]->val, g->inputs[1]->val);
            g->val.swap(nand_result);
        }

        for (auto out_g : g->outputs) {
            q.push(out_g);
        }
    }
}

