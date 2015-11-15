# GSW Fully homomorphic encryption implementation

This is the code part of my final year Computer Science BSc project at the
University of Bristol.

## Requirements

- CMake
- NTL
- GMP

## Building

```
mkdir -f build
cd build
cmake ..
make
```

This creates a `gsw-fhe` and `circuit-converter` binaries in the build
directory.  Usage instructions can be printed with `-h` flag.

## Tests

There's some tests in `test` directory written using pyunit. They're only
really useful in the future (i.e. 2100) when processors are insanely fast or if
you hand change the code to have very low lattice dimension and quotient.

## License

This code is released under MIT License.

The usual excuses of "most of this code was written to never be looked at
again" apply.
