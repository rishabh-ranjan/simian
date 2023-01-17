# Exploiting Epochs and Symmetries in Analysing MPI Programs

Simian is a prototype tool for epoch- and symmetry-aware verification of message-passing programs for communication deadlocks. It supports C and C++ programs written using the Message Passing Interface (MPI). Simian implements the techniques in our paper ["Exploiting Epochs and Symmetries in Analysing MPI Programs"](https://dl.acm.org/doi/abs/10.1145/3551349.3556954), which was accepted at the _International Conference on Automated Software Engineering (ASE) 2022_.

## Dependencies:

MPICH: `sudo apt install mpich`

Z3: `sudo apt install libz3-dev`

LLVM-3.8.0: install `clang+llvm-3.8.0-x86_64-linux-gnu-debian8` into a directory of the same name inside clangTool directory from <https://releases.llvm.org/3.8.0/>

BLISS-0.73: install from <http://www.tcs.hut.fi/Software/bliss/> into directory `scheduler/simian-core/bliss-0.73`

XDOT: (optional -- to render generated .dot files) `sudo apt install xdot`

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

You may have to edit the path to Simian in `instrument.sh`. The tool has been tested to work on Ubuntu 20.04.

## Citation

```
@inproceedings{simian,
  title={Exploiting Epochs and Symmetries in Analysing {MPI} Programs},
  author={Ranjan, Rishabh and Agrawal, Ishita and Sharma, Subodh},
  booktitle={37th IEEE/ACM International Conference on Automated Software Engineering},
  year={2022}
}
```
