# Type make, then type ./compile c.txt
compile = compile.c ../src/kinds.c

# C compiler and options for debugging or optimisation. On macOS, 'gcc'
# refers to 'clang'.
GCC = gcc -std=c11 -pedantic -Wall -Wextra
DEBUG = -g -fsanitize=undefined -fsanitize=address
OPT = -O2 -flto

compile: compile.c
	@ $(GCC) $(DEBUG) -DTEST -DTEST$@ $($@) -o compile
