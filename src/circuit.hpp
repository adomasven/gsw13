#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <memory>
#include <stdexcept>
#include <cstdint>

typedef enum {AND, XOR, INV, NAND, VAL} GateType;

template <typename T>
struct Gate {
    GateType type;
    std::vector<std::shared_ptr<Gate> > inputs;
    std::vector<std::shared_ptr<Gate> > outputs;
    T val;
    intmax_t id;
};

template <typename T>
class CircuitBase {
public:
    std::vector<std::shared_ptr<Gate<T> > > inputs;
    std::vector<std::shared_ptr<Gate<T> > > outputs;
    uintmax_t num_gates, num_wires, num_in1, num_in2, num_out;

    CircuitBase() : CircuitBase(std::cin) {};
    CircuitBase(std::istream& fp) {
        init(fp);
    };
    CircuitBase(std::string filename) {
        std::ifstream fp(filename.c_str());
        if(! fp.is_open()) {
            throw std::runtime_error("Bad file: " + filename);
        }
        init(fp);
        fp.close();
    };

    void init(std::istream& fp) {
        using namespace std;
        map<uintmax_t, shared_ptr<Gate<T> > > gate_map;

        fp >> num_gates >> num_wires >> num_in1 >> num_in2 >> num_out;

        for (uintmax_t i = 0; i < num_wires; i++) {
            shared_ptr<Gate<T> > g(new Gate<T> ); g->type = VAL; g->id = -1;
            if (i < num_in1 + num_in2) {
                inputs.push_back(g);
            } else if (i >= num_wires - num_out) {
                outputs.push_back(g);
            }
            gate_map[i] = g;
        }
        for (uintmax_t i = 0; i < num_gates; i++) {
            int num_inputs, num_outputs, in1, in2, out;
            fp >> num_inputs >> num_outputs;
            if (num_inputs == 2) {
                fp >> in1 >> in2 >> out;
            } else {
                fp >> in1 >> out;
                in2 = -1;
            }
            string type;
            fp >> type;
            shared_ptr<Gate<T> > g = gate_map.at(out); g->id = -1;
            if (! type.compare("XOR")) {
                g->type = XOR;
            } else if (! type.compare("AND")) {
                g->type = AND;
            } else if (! type.compare("INV")) {
                g->type = INV;
            } else if (! type.compare("NAND")) {
                g->type = NAND;
            }

            gate_map.at(in1)->outputs.push_back(g);
            g->inputs.push_back(gate_map.at(in1));
            if (in2 != -1) {
                if (in1 != in2) {
                    gate_map.at(in2)->outputs.push_back(g);
                }
                g->inputs.push_back(gate_map.at(in2));
            }
        }
    };

    void output(std::ostream& fp) {
        using namespace std;
        reset();

        fp << num_gates << "\t" << num_wires << endl;
        fp << num_in1 << "\t" << num_in2 << "\t" << num_out << endl << endl;

        uint64_t id = num_wires - outputs.size();
        // Initialize output ids, because they have to be the last num_wires numbers
        for (auto g : outputs) {
            g->id = id++;
        }
        id = 0;
        
        // Initialize queue for gate output
        queue<shared_ptr<Gate<T> > > q;
        for (auto g : inputs) {
            g->id = id++;
            for (auto out_g : g->outputs) {
                q.push(out_g);
            }
        }
        
        set<shared_ptr<Gate<T> >> already_printed;
        while (!q.empty()) {
            shared_ptr<Gate<T> > g = q.front(); q.pop();
            if (g->id == -1) {
                g->id = id++;
            }
            if (already_printed.find(g) != already_printed.end()) {
                continue;
            }
            already_printed.insert(g);

            fp << g->inputs.size() << "\t" << 1 << "\t";
            for (auto in_g : g->inputs) {
                if(in_g->id == -1) {
                    in_g->id = id++;
                }
                fp << in_g->id << "\t";
            }
            fp << g->id << "\t";
            switch (g->type) {
                case AND: fp << "AND"; break;
                case XOR: fp << "XOR"; break;
                case INV: fp << "INV"; break;
                case NAND: fp << "NAND"; break;
                default: throw runtime_error("Trying to print unknown gate");
            }
            fp << endl;

            for (auto out_g : g->outputs) {
                q.push(out_g);
            }

        }
    };

    uint64_t depth() {
        using namespace std;
        uint64_t depth = 0;
        set<shared_ptr<Gate<T> > > seen;
        vector<shared_ptr<Gate<T> > > layer;
        for (auto g : inputs) {
            if (seen.find(g) != seen.end()) {
                continue;
            }
            layer.push_back(g);
            seen.insert(g);
        }
        while (!layer.empty()) {
            vector<shared_ptr<Gate<T> > > next_layer;
            for (auto g : layer) {
                for (auto out_g : g->outputs) {
                    if (seen.find(out_g) != seen.end()) {
                        continue;
                    }
                    next_layer.push_back(out_g);
                    seen.insert(out_g);
                }
            }
            layer = next_layer;
            depth++;
        }
        return depth;
    }
    virtual void reset()=0;
};

class Circuit : public CircuitBase<int8_t> {
public:
    Circuit();
    Circuit(std::string);
    Circuit(std::istream&);

    void reduce(std::vector<bool>, uint32_t);
    void nand_recode();
    void reset();
    void eval(std::vector<int8_t>);
private:
    uint8_t and_to_nand(std::shared_ptr<Gate<int8_t> >);
    uint8_t xor_to_nand(std::shared_ptr<Gate<int8_t> >);
    uint8_t inv_to_nand(std::shared_ptr<Gate<int8_t> >);

    void replace_inputs(std::shared_ptr<Gate<int8_t> >, std::shared_ptr<Gate<int8_t> >);
};


