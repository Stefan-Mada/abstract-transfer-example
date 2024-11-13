# Abstract Transfer Functions Analysis

This repository implements a naive approach to calculating known bits information,
in an attempt to show that LLVM's transfer functions, while sound, are not
optimal. 

That is, LLVM Known Bits transfer functions, specifically for saturated signed addition,
leave possible precision on the table. Given aF, an abstract transfer function from and to
the abstract known bits domain, cF, a concrete transfer function from the APInt domain
to APInts, y, a concretization function that takes values from the known bits domain and
outputs a set of concrete APInts, and a, an abstraction function that takes values from
a set of concrete APInts and generates the most precise known bits representation, optimality
is the property that:

aF(x) = a(cF(y(x))) : where x is a concrete known bits value

However, this program shows that aF(x) is strictly less precise than the naive approach,
showing that LLVM's known bits transfer function for saturated signed addition is not optimal.

## Running
To build, run these commands:
> mkdir build
> 
> cd build
> 
> cmake -DLLVM_DIR=/<your llvm build folder here>/lib/cmake/llvm -GNinja ..
> 
> ninja

To run, simply do:
> ./abstract-transfer.out <n>

Where n is the bitwidth you wish to analyze. Note, the naive approach is a double exponential,
so beyond abitwidth of 6, it will take a very long time.