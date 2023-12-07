# The modules and their dependencies.
array = array.c
types = types.c

# Debugging options (remove if not available)
DEBUG = -fsanitize=address -fsanitize=undefined

# Default rule for building and testing a module
%: %.c
	gcc -std=c11 -o run -Wall -pedantic -g $(DEBUG) -D$@Test $($@)
	./run
