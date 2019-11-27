style = style.c

# Default rule for building and testing a module. (Use gcc or clang.)
%: %.c
	gcc -std=c11 -o snipe -Wall -pedantic -g $(DEBUG) -D$@Test $($@)
	./snipe

# Find the OS platform using the uname command.
Linux := $(findstring Linux, $(shell uname -s))
MacOS := $(findstring Darwin, $(shell uname -s))
Windows := $(findstring NT, $(shell uname -s))

# Advanced debugging options (not easily available on Windows)
ifdef Windows
	DEBUG =
else
	DEBUG = -fsanitize=address -fsanitize=undefined
endif
