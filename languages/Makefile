# Definitive list of compiler modules in dependency order.
scan = scan.c
scan2 = scan2.c

# C compiler and options for debugging or optimisation. On macOS, 'gcc'
# refers to 'clang'.
GCC = gcc -std=c11 -pedantic -Wall -Wextra
DEBUG = -g -fsanitize=undefined -fsanitize=address
OPT = -O2 -flto

# Generic rule for compiling and testing any module.
%: %.c
	@ $(GCC) $(DEBUG) -DTEST -DTEST$@ $($@) -o test
	@ ./test
	@ rm ./test
