# The modules and their dependencies. (Header files are not included, because
# incremental compiling is not used. All the modules relevant to testing a
# module are compiled together.)
array = array.c
bytes = bytes.c
types = types.c
brackets = brackets.c types.c array.c
tokens = tokens.c array.c

# Debugging options (remove if not available)
DEBUG = -fsanitize=address -fsanitize=undefined

# Default rule for building and testing a module.
# @ means silent; use make -n to see the generated commands.
%: %.c
	@gcc -std=c11 -o run -Wall -pedantic -g $(DEBUG) -D$@Test $($@)
	@./run
