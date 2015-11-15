#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <vector>
#include <string>
#include <cstdlib>
#include <argp.h>

#include "utils.hpp"
#include "gsw.hpp"
#include "circuit.hpp"
#include "cryptoCircuit.hpp"


using namespace std;


const char *argp_program_version = "GSW FHE 0.1";
const char *argp_program_bug_address = "<av13833@my.bristol.ac.uk>";

static char doc[] =
    "gsw-fhe -- a FHE implementation based on GSW scheme";

static char args_doc[] = "";

static struct argp_option options[] = {
    {"keygen",        'k', "int",     OPTION_ARG_OPTIONAL, "Generate a public and secret key pair. Set optional kappa value. Default 80"},
    {"circuit_depth", 'L', "int",     0,                   "Circuit depth. Required with -k"},
    {"encrypt",       'e', 0,         0,                   "Encrypt using public key"},
    {"decrypt",       'd', 0,         0,                   "Decrypt using secret key"},
    {"nand",          'n', 0,         0,                   "NAND two ciphertexts together"},
    {"circuit",       'c', "FILE",    0,                   "A NAND circuit description file"},
    {"public_key",    'p', "FILE",    0,                   "Public key file"},
    {"secret_key",    's', "FILE",    0,                   "Secret key file"},
    {"output",        'o', "FILE",    0,                   "Output to file instead of STDOUT"},
    {"input",         'i', "FILE",    0,                   "Input from file instead of STDIN"},
    {0}
};

struct arguments_t {
    char *input_file, *output_file, *public_key, *secret_key, *circuit;
    bool keygen, encrypt, decrypt, nand;
    int kappa, circuit_depth;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    arguments_t *arguments = (arguments_t *) state->input;

    switch(key) {
        case 'k': arguments->keygen = true; arguments->kappa = arg ? atoi(arg) : 80; break;
        case 'L': arguments->circuit_depth = atoi(arg); break; 
        case 'e': arguments->encrypt = true; break;
        case 'd': arguments->decrypt = true; break;
        case 'n': arguments->nand = true; break;
        case 'c': arguments->circuit = arg; break;
        case 'p': arguments->public_key = arg; break;
        case 's': arguments->secret_key = arg; break;
        case 'o': arguments->output_file = arg; break;
        case 'i': arguments->input_file = arg; break;
        case ARGP_KEY_ARG: argp_usage(state); break;
        case ARGP_KEY_END:
            if (arguments->encrypt && arguments->decrypt)
                argp_error(state, "Cannot both encrypt and decrypt");
            if (arguments->encrypt && arguments->public_key == NULL)
                argp_error(state, "Must provide public_key");
            if (arguments->decrypt && arguments->secret_key == NULL)
                argp_error(state, "Must provide secret_key");
            if (arguments->keygen && (
                        ! (arguments->circuit_depth || arguments->circuit) || 
                        ! arguments->public_key ||
                        ! arguments->secret_key))
                argp_error(state, "Must provide circuit_depth/circuit, public_key and private_key arguments");
            if (! (arguments->encrypt || arguments->decrypt || arguments->keygen || arguments->nand || arguments->circuit)) 
                argp_error(state, "Invalid input");
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

void write_keys(const arguments_t &arguments, const GSW &gsw) {
    const char *sk_output = 
        "-----BEGIN GSW SECRET KEY-----\n"
        "%i\n" // n
        "%i\n" // m
        "%s\n" // q
        "%s\n" // sk
        "-----END GSW SECRET KEY-----\n"
        ;
    const char *pk_output = 
        "-----BEGIN GSW PUBLIC KEY-----\n"
        "%i\n" // n
        "%i\n" // m
        "%s\n" // q
        "%s\n" // pk
        "-----END GSW PUBLIC KEY-----\n"
        ;
    FILE *fpk, *fsk;
    std::stringstream ssk, spk, q;
    BIVector sk = gsw.secret_key_gen();
    BIMatrix pk = gsw.public_key_gen(sk);

    ssk << sk;
    spk << pk;
    q << gsw.quotient;

    fsk = fopen(arguments.secret_key, "w");
    fpk = fopen(arguments.public_key, "w");

    fprintf(fsk, sk_output, gsw.n, gsw.m, q.str().c_str(), ssk.str().c_str());
    fprintf(fpk, pk_output, gsw.n, gsw.m, q.str().c_str(), spk.str().c_str());

    fclose(fpk);
    fclose(fsk);
}

BIMatrix read_key(const char* file_path, GSW &gsw) {
    BIMatrix key;
    BigInt q;
    string tmp, skey;
    unsigned int n, m;
    std::ifstream file(file_path);

    if (!file.good()) {
        throw ex("Key file not found");
    }

    std::getline(file, tmp);
    if (!std::regex_match(tmp, std::regex("-----BEGIN GSW (SECRET|PUBLIC) KEY-----"))) {
        throw ex("Invalid key file");
    }

    file >> n >> m >> q;

    getline(file, tmp); // flush endline character
    getline(file, skey);
    getline(file, tmp);
    if (!std::regex_match(tmp, std::regex("-----END GSW (SECRET|PUBLIC) KEY-----"))) {
        throw ex("Invalid key file");
    }
    file.close();

    // use extracted params
    gsw.n = n;
    gsw.n_1 = n + 1;
    gsw.m = m;
    gsw.quotient = q;
    gsw.l = floor(log(q)/log(2)) + 1;
    gsw.N = (n + 1) * gsw.l;

    stringstream sskey(skey);
    string key_bit;
    while(sskey >> key_bit) {
        key.push_back(NTL::conv<NTL::ZZ>(key_bit.c_str()));
    }
    return key;
}

vector<bool> read_plaintexts(const char* input) {
    vector<bool> plaintexts;
    std::istream* fp = &std::cin;
    std::ifstream fin;
    if (input) {
        fin.open(input);
        fp = &fin;
    }
    bool val;
    while(*fp >> val) {
        plaintexts.push_back(val);
    }
    return plaintexts;
}

vector<BitMatrix> read_ciphertexts(const char* input) {
    vector<BitMatrix> ciphertexts;
    std::istream* fp = &std::cin;
    std::ifstream fin;
    if (input) {
        fin.open(input);
        fp = &fin;
    }
    string val;
    while(*fp >> val) {
        BitMatrix ciphertext;
        for (unsigned int i = 0; i < val.size(); i++) {
            ciphertext.push_back(val[i] == '1');
        }
        ciphertexts.push_back(ciphertext);
    }
    return ciphertexts;
}

void write_plaintexts(const char* output, const vector<bool> plaintexts) {
    std::ostream* fp = &std::cout;
    std::ofstream fout;
    if (output) {
        fout.open(output);
        fp = &fout;
    }
    for(auto it = plaintexts.begin(); it != plaintexts.end(); ++it) {
        *fp << *it << "\n";
    }
}

void write_ciphertexts(const char* output, const vector<BitMatrix> ciphertexts) {
    std::ostream* fp = &std::cout;
    std::ofstream fout;
    if (output) {
        fout.open(output);
        fp = &fout;
    }
    for(auto it = ciphertexts.begin(); it != ciphertexts.end(); ++it) {
        *fp << *it << "\n";
    }
}

vector<BitMatrix> encrypt_plaintexts(const vector<bool> plaintexts, const BIVector key, const GSW &gsw) {
    BigInt val;
    vector<BitMatrix> ciphertexts;
    for (auto it = plaintexts.begin(); it != plaintexts.end(); ++it) {
        val = *it;
        BitMatrix ciphertext = gsw.encrypt(key, val);
        ciphertexts.push_back(ciphertext);
    }
    return ciphertexts;
}

vector<bool> decrypt_ciphertexts(const vector<BitMatrix> ciphertexts, const BIVector key, const GSW &gsw) {
    BitMatrix val;
    vector<bool> plaintexts;
    for (auto it = ciphertexts.begin(); it != ciphertexts.end(); ++it) {
        val = *it;
        bool plaintext = gsw.decrypt_bit(key, val);
        plaintexts.push_back(plaintext);
    }
    return plaintexts;
}

vector<BitMatrix> nand_ciphertexts(vector<BitMatrix> ciphertexts, const GSW &gsw) {
    vector<BitMatrix> res;
    res.push_back(gsw.nand(ciphertexts[0], ciphertexts[1]));
    return res;
}

int main(int argc, char **argv) {
    arguments_t arguments = {0};
    BIMatrix key;
    vector<bool> plaintexts;
    vector<BitMatrix> ciphertexts;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    utils_init();


    if(!arguments.circuit_depth && arguments.circuit) {
    	Circuit circuit(arguments.circuit);
    	circuit.nand_recode();
    	arguments.circuit_depth = circuit.depth();
    }

    GSW gsw = GSW(arguments.kappa, arguments.circuit_depth);

//    BigInt message;
//    message = 6;
//    const auto secret_key = gsw.secret_key_gen();
//    const auto public_key = gsw.public_key_gen(secret_key);
//    const auto cyphertext = gsw.encrypt(public_key, message);
//    const auto m = gsw.decrypt(secret_key, cyphertext);
//#define DEBUG
//#ifdef DEBUG
////    for (int i = 0; i < 10; i++) {
////      std::cout << public_key[i] << " " << secret_key[i] << " " << cyphertext[i];
//      std::cout << cyphertext;
//      std::cout << std::endl;
////    }
//#endif
//    std::cout << message << " " << m << std::endl << std::endl;
//    return 0;


    if (arguments.keygen) {
        write_keys(arguments, gsw);
        return 0;
    }

    if (arguments.secret_key) {
        key = read_key(arguments.secret_key, gsw);
    }
    else if (arguments.public_key) {
        key = read_key(arguments.public_key, gsw);
    }

    if (arguments.encrypt) {
        plaintexts = read_plaintexts(arguments.input_file);
        ciphertexts = encrypt_plaintexts(plaintexts, key, gsw);
        write_ciphertexts(arguments.output_file, ciphertexts);
    } 
    else if (arguments.decrypt) {
        ciphertexts = read_ciphertexts(arguments.input_file);
        plaintexts = decrypt_ciphertexts(ciphertexts, key, gsw);
        write_plaintexts(arguments.output_file, plaintexts);
    } 
    else if (arguments.nand) {
        ciphertexts = read_ciphertexts(arguments.input_file);
        ciphertexts = nand_ciphertexts(ciphertexts, gsw);
        write_ciphertexts(arguments.output_file, ciphertexts);
    } else if (arguments.circuit) {
        CryptoCircuit circuit(arguments.circuit);
        ciphertexts = read_ciphertexts(arguments.input_file);
        circuit.eval(ciphertexts, gsw);
        ciphertexts.clear();
        for (auto g : circuit.outputs) {
            ciphertexts.push_back(g->val);
        }
        write_ciphertexts(arguments.output_file, ciphertexts);
    }


    return 0;
}

