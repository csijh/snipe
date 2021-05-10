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

FLAGS = -std=c11 -Wall -pedantic
DEBUG = -g -fsanitize=undefined -fsanitize=address -fsanitize=leak
#DEBUG = -g

compile2: compile2.c
	gcc $(FLAGS) $(DEBUG) compile2.c \
	rules.c states.c patterns.c strings.c -o compile2

compile: compile.c
	gcc $(FLAGS) $(DEBUG) compile.c -o compile
	cp compile ../compile

interpret: interpret.c
	gcc -std=c11 -Wall -pedantic $(DEBUG) interpret.c -o interpret
	cp interpret ../interpret

#match: match.c
#	gcc -std=c11 -Wall -pedantic $(DEBUG) match.c -o match

# Default rule for building and testing a module.
%: %.c
	gcc $(FLAGS) $(DEBUG) -D$@Test $($@) -o test
	./test
	rm test
