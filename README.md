# Simian: Symmetry-aware deadlock detection for MPI programs

## Dependencies:

MPICH: `sudo apt install mpich`

Z3: `sudo apt install libz3-dev`

LLVM-3.8.0: install `clang+llvm-3.8.0-x86_64-linux-gnu-debian8` into a directory of the same name inside clangTool directory from <https://releases.llvm.org/3.8.0/>

BLISS-0.73: install from <http://www.tcs.hut.fi/Software/bliss/> into directory `scheduler/simian-core/bliss-0.73`

XDOT: (to render generated .dot files) `sudo apt install xdot`

## Installation:

```
autoreconf --force
./configure
make
sudo make install
```

## Example Run:

```
cd scheduler/benchmarks/adder
../instrument.sh adder.c
simian -n 8 ./i_adder
```

## TODO:

1. Complete benchmarking for remaining benchmarks

2. Collect more benchmarks

3. Make detailed README

4. Add LICENSE compliant with the external software licenses

## Future work:

1. Improve Hermes frontend (clangTool):
	1.1. can handle single source file programs only
	1.2. variables names used in interumentation code are too common -- clashes with the identifiers in the input program
	1.3. can handle only very simple if conditions
	1.4. static analysis used is very fragile

2. Generate deadlocking trace from SMT solution

3. Map detected deadlock to relevant code sections (epochs can help to localize the bug)

4. User-friendly interface (GUI) to explore the deadlocks/epochs/symmetries present in the program in a graphical manner

5. Implement support for conditional matches-before order of MPI semantics
