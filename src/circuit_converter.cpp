#include <vector>
#include <iostream>
#include <string>
#include <argp.h>

#include "circuit.hpp"

const char *argp_program_version = "Boolean Logic Circuit Converter 0.1";
const char *argp_program_bug_address = "<av13833@my.bristol.ac.uk>";

static char doc[] =
    "circuit-converter -- convert and simply boolean circuits";

static char args_doc[] = "";

static struct argp_option options[] = {
    {"simplify",      's', "<pattern>",                0,   "Simplify a circuit."},
    {"nand",          'n', 0,                          0,   "Convert to NAND based circuit"},
    {0}
};

struct arguments_t {
    char *simplification;
    int in1;
    bool simplify, nand;

};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    arguments_t *arguments = (arguments_t *) state->input;

    switch(key) {
        case 'n': arguments->nand = true; break;
        case 's': arguments->simplify = true; arguments->simplification = arg; break;
        case ARGP_KEY_ARG: 
            if (!arguments->simplify)
                argp_usage(state);
            else if (state->arg_num == 0)
                arguments->in1 = atoi(arg);
            else
                return ARGP_ERR_UNKNOWN;

            break;
        case ARGP_KEY_END:
            if (arguments->simplify && ! (arguments->in1))
                argp_error(state, "Simplification requires num_in1");
            
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc, char** argv) {
    arguments_t arguments = {0};

    argp_parse(&argp, argc, argv, 0, 0, &arguments);
    
    Circuit c = Circuit();

    if (arguments.nand) {
        c.nand_recode();
    } else {
        std::vector<bool> out(c.outputs.size(), false);
        std::string pattern = arguments.simplification;
        for (size_t i = 0; i < pattern.size(); i++) {
            char val = pattern[i] - '0';
            if (val < 0 || val > 1) {
                throw std::runtime_error("Pattern values can only be 0 or 1");
            }
            out[i] = val;
        }
        c.reduce(out, arguments.in1);
    }
    c.output(std::cout);
}
