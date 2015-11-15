#!/usr/bin/env python3.5

import subprocess as sp
import unittest


def gen_key(pub, priv):
    return sp.run(['../build/gsw-fhe', '-k', '-L', '1', '-p', pub, '-s', priv])

def encrypt(key, input_file, output_file):
    return sp.run(['../build/gsw-fhe', '-e', '-p', key, '-i', input_file, '-o', output_file])

def decrypt(key, input_file, output_file):
    return sp.run(['../build/gsw-fhe', '-d', '-s', key, '-i', input_file, '-o', output_file])

def nand(input_file, output_file):
    return sp.run(['../build/gsw-fhe', '-n', '-i', input_file, '-o', output_file])

def run_circuit(circuit, input_file, output_file):
    return sp.run(['../build/gsw-fhe', '-c', circuit, '-i', input_file, '-o', output_file])

def diff_files(a, b):
    return sp.run(['diff', a, b], stdout=sp.PIPE).returncode

class GSWTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        gen_key('key.pub', 'key')

    @classmethod
    def tearDownClass(cls):
        return
        sp.run(['rm', 'output', 'ciphertext'])


class EncryptionTest(GSWTest):
    def test_encryption(self):
        encrypt('key.pub', 'input', 'ciphertext')
        decrypt('key', 'ciphertext', 'output')
        self.assertEqual(diff_files('input', 'output'), 0)

class NandTest(GSWTest):
    @classmethod
    def setUpClass(cls):
        super().setUpClass()

        cls.inputs = ['00', '01', '10', '11']
        cls.results = ['1', '1', '1', '0']

        for s in cls.inputs:
            with open('in{}'.format(s), 'w') as fp:
                fp.write('\n'.join(list(s)))

    @classmethod
    def tearDownClass(cls):
        sp.run(['rm', 'output', 'ciphertext'])
        for s in cls.inputs:
            sp.run(['rm', 'in{}'.format(s)])


    def test_nand(self):
        for i, s in enumerate(self.inputs):
            encrypt('key.pub', 'in{}'.format(s), 'ciphertext')
            nand('ciphertext', 'ciphertext')
            decrypt('key', 'ciphertext', 'output')
            self.assertEqual(chr(sp.run(['cat', 'output'], stdout=sp.PIPE).stdout[0]), self.results[i], s)

class Adder1BitTest(GSWTest):
    @classmethod
    def setUpClass(cls):
        super().setUpClass()

        cls.inputs = ['00', '01', '10', '11']
        cls.results = ['0', '1', '1', '0']

        for s in cls.inputs:
            with open('in{}'.format(s), 'w') as fp:
                fp.write('\n'.join(list(s)))

        cls.genCircuit()

    @classmethod
    def tearDownClass(cls):
        sp.run(['rm', 'output', 'ciphertext', 'circuit'])
        for s in cls.inputs:
            sp.run(['rm', 'in{}'.format(s)])

    @classmethod
    def genCircuit(cls, out=1):
        with open('adder_32bit.txt', 'r') as adderf:
            res = sp.run(['../build/circuit-converter', '-s', '1', str(out+1)], stdin=adderf, stdout=sp.PIPE, universal_newlines=True)
        with open('circuit', 'w') as circuitf:
            res = sp.run(['../build/circuit-converter', '-n'], input=res.stdout, stdout=circuitf, universal_newlines=True)

    def test_add(self):
        for i, s in enumerate(self.inputs):
            encrypt('key.pub', 'in{}'.format(s), 'ciphertext')
            run_circuit('circuit', 'ciphertext', 'ciphertext')
            decrypt('key', 'ciphertext', 'output')
            output = sp.run(['cat', 'output'], stdout=sp.PIPE, universal_newlines=True).stdout[:-1]
            for j, b in enumerate(output):
                self.assertEqual(b, self.results[i][j], s)


if __name__ == '__main__':
    unittest.main()
