# To test out a new definition for language x:
#    make compile;    cd ..;  ./compile x
#    make interpret;  cd ..;  ./interpret x
# To test a module, make one of these.
strings = strings.c
tags = tags.c $(strings)
patterns = patterns.c $(strings)
states = states.c $(strings)
rules = rules.c $(strings)

match = match.c $(strings)
quads = quads.c $(tags)

#DEBUG = -g
DEBUG = -g -fsanitize=undefined -fsanitize=address -fsanitize=leak

compile: compile.c
	gcc -std=c11 -Wall -pedantic $(DEBUG) compile.c -o compile
	cp compile ../compile

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
