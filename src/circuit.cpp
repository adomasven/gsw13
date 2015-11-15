#include <algorithm>

#include "circuit.hpp"

using namespace std;

Circuit::Circuit() : CircuitBase() { }
Circuit::Circuit(string filename) : CircuitBase(filename) { }
Circuit::Circuit(istream& fp) : CircuitBase(fp) { }

void Circuit::reset() {
    queue<shared_ptr<Gate<int8_t> > > q;
    for (auto g : inputs) {
        g->id = g->val = -1;
        for (auto out_g : g->outputs) {
            q.push(out_g);
        }
    }
    while (!q.empty()) {
        shared_ptr<Gate<int8_t> > g = q.front();
        q.pop();
        g->id = g->val = -1;
        for (auto out_g : g->outputs) {
            q.push(out_g);
        }
    }
}

void Circuit::eval(vector<int8_t> in) {
    reset();
    queue<shared_ptr<Gate<int8_t> > > q;
    for (uintmax_t i = 0; i < inputs.size(); i++) {
        inputs[i]->val = in[i];
        for (auto out_g : inputs[i]->outputs) {
            q.push(out_g);
        }
    }
    while (!q.empty()) {
        shared_ptr<Gate<int8_t> > g = q.front();
        q.pop();

        bool ready = true;
        for (auto in_g : g->inputs) {
            ready = ready && in_g->val != -1;
        }
        // Not all input values propagated yet
        if (!ready) {
            continue;
        }

        if (g->type == XOR) {
            g->val = g->inputs[0]->val ^ g->inputs[1]->val;
        } else if (g->type == AND) {
            g->val = g->inputs[0]->val & g->inputs[1]->val;
        } else if (g->type == INV) {
            g->val = ! g->inputs[0]->val;
        } else if (g->type == NAND) {
            g->val = !(g->inputs[0]->val & g->inputs[1]->val);
        }
        for (auto out_g : g->outputs) {
            q.push(out_g);
        }
    }
}

void Circuit::reduce(vector<bool> out, uint32_t in1) {
    reset();

    // Mark alive
    set<shared_ptr<Gate<int8_t> > > alive;
    queue<shared_ptr<Gate<int8_t> > > q;
    for (uintmax_t i = 0; i < outputs.size(); i++) {
        if (out[i]) {
            for (auto in_g : outputs[i]->inputs) {
                q.push(in_g);
            }
            alive.insert(outputs[i]);
        }
    }

    while (!q.empty()) {
        shared_ptr<Gate<int8_t> > g = q.front();
        q.pop();

        if (alive.find(g) != alive.end()) {
            continue;
        }
        alive.insert(g);

        for (auto in_g : g->inputs) {
            q.push(in_g);
        }
    }

    // Remove dead
    for (auto g : alive) {
        g->outputs.erase(
            remove_if(
                begin(g->outputs), end(g->outputs), 
                [&alive](shared_ptr<Gate<int8_t> > out_g)
                {
                    return alive.find(out_g) == alive.end();
                }
            ), g->outputs.end()
        );
        
    }
    
    // Remove dead inputs and outputs
    inputs.erase(
        remove_if(
            begin(inputs), end(inputs), 
            [alive](shared_ptr<Gate<int8_t> > g){return alive.find(g) == alive.end();}
        ), inputs.end()
    );
    outputs.erase(
        remove_if(
            begin(outputs), end(outputs), 
            [&alive](shared_ptr<Gate<int8_t> > g){return alive.find(g) == alive.end();}
        ), outputs.end()
    );


    num_out = outputs.size();
    num_in1 = inputs.size() >= in1 ? in1 : inputs.size();
    num_in2 = inputs.size() - in1;
    num_gates = alive.size() - inputs.size();
    num_wires = alive.size();
}

void Circuit::nand_recode() {
    queue<shared_ptr<Gate<int8_t> > > q;
    for (auto g : inputs) {
        for (auto out_g : g->outputs) {
            q.push(out_g);
        }
    }
    set<shared_ptr<Gate<int8_t> > > already_traversed;
    while (!q.empty()) {
        shared_ptr<Gate<int8_t> > g = q.front();
        q.pop();
        if (already_traversed.find(g) != already_traversed.end()) {
            continue;
        }
        already_traversed.insert(g);

        uint8_t num_new_gates = 0;
        switch (g->type) {
            case AND: num_new_gates = and_to_nand(g); break;
            case XOR: num_new_gates = xor_to_nand(g); break;
            case INV: num_new_gates = inv_to_nand(g); break;
            default: break;
        }
        num_wires += num_new_gates;
        num_gates += num_new_gates;

        for (auto out_g : g->outputs) {
            q.push(out_g);
        }
    }
}

uint8_t Circuit::and_to_nand(std::shared_ptr<Gate<int8_t> > g2) {
    std::shared_ptr<Gate<int8_t> > g1(new Gate<int8_t> );
    g1->type = g2->type = NAND;
    g1->val = g2->val = -1;

    g1->outputs.push_back(g2);
    g1->inputs = g2->inputs;
    replace_inputs(g2, g1);

    g2->inputs.clear();
    g2->inputs.push_back(g1);
    g2->inputs.push_back(g1);

    return 1;
}

uint8_t Circuit::inv_to_nand(std::shared_ptr<Gate<int8_t> > inv) {
    inv->type = NAND;
    inv->val = -1;

    inv->inputs.push_back(inv->inputs[0]);

    return 0;
}

uint8_t Circuit::xor_to_nand(std::shared_ptr<Gate<int8_t> > end) {
    std::shared_ptr<Gate<int8_t> > 
        start(new Gate<int8_t> ), 
        g1(new Gate<int8_t> ), 
        g2(new Gate<int8_t> );

    start->type = end->type = g1->type = g2->type = NAND;
    start->val = end->val = g1->val = g2->val = -1;

    start->inputs = end->inputs;
    start->outputs.push_back(g1);
    start->outputs.push_back(g2);
    replace_inputs(end, start);

    g1->inputs.push_back(end->inputs[0]);
    g1->inputs.push_back(start);
    g1->outputs.push_back(end);

    g2->inputs.push_back(end->inputs[1]);
    g2->inputs.push_back(start);
    g2->outputs.push_back(end);

    end->inputs[0]->outputs.push_back(g1);
    end->inputs[1]->outputs.push_back(g2);
    end->inputs.clear();
    end->inputs.push_back(g1);
    end->inputs.push_back(g2);
    end->outputs = end->outputs;

    return 3;
}

void Circuit::replace_inputs(std::shared_ptr<Gate<int8_t> > old, std::shared_ptr<Gate<int8_t> > new_g) {
    for (auto in_g : old->inputs) {
        for (uint8_t i = 0; i < in_g->outputs.size(); i++) {
            if (in_g->outputs[i] == old) {
                in_g->outputs[i] = new_g;
            }
        }
    }
}

