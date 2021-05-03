# To test out a new definition for language x:
#    make interpret;  ./interpret ../x
# To test a module, make one of these.
stacks = stacks.c
tags = tags.c $(stacks)
matcher = matcher.c $(tags)

match = match.c $(strings)
quads = quads.c $(tags)

#DEBUG = -g
DEBUG = -g -fsanitize=undefined -fsanitize=address -fsanitize=leak

interpret: interpret.c
	gcc -std=c11 -Wall -pedantic $(DEBUG) interpret.c -o interpret
	cp interpret ../interpret

#match: match.c
#	gcc -std=c11 -Wall -pedantic $(DEBUG) match.c -o match

# Default rule for building and testing a module.
%: %.c
	clang $(DEBUG) -D$@Test $(FLAGS) $($@) -o test
	./test
	rm test
