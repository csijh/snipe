# The modules and their dependencies. (Header files are not included, because
# incremental compiling is not used. All the modules relevant to testing a
# module are compiled together.)
array = array.c
unicode = unicode.c
file = file.c unicode.c array.c
kinds = kinds.c
text = text.c kinds.c
brackets = brackets.c text.c kinds.c
lines = lines.c text.c kinds.c

# Debugging options (remove if not available)
DEBUG = -fsanitize=address -fsanitize=undefined

# Default rule for building and testing a module.
# @ means silent; use make -n to see the generated commands.
%: %.c
	@gcc -std=c11 -o run -Wall -pedantic -g $(DEBUG) -D$@Test $($@)
	@./run
