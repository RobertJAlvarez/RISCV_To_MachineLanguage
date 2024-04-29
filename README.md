# RISC-V code to Machine Language
A C++ program which converts RISC-V assembly code to Machine Code along with Data Memory.
Conversions are done based on RISC-V reference sheet provided in this repository.

# Instructions implemented
Currently supports all instructions available in Format.txt file.
Load Address and Load Word pseudo-instructions are also implemented.

# How to build program (In Compiler)
->	Place code to translate in `test.asm` \
->  Type `make run` \
->  Machine code generated for `test.asm`can be found in `MCode.mc` \
-> `MCode.md` would also contain the data memory.

# Automatic MC generator
->  If code is modified, run `./gen.sh` \
->  Such command would generate MC for all files in `Test_Cases` \
->  Follow by a comparison of all the output with the files in `org_output` which are correct.

