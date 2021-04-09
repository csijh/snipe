# To test out a new definition for language x:
#    make compile
#    ./compile x
#    make interpret
#    ./interpret x

#DEBUG = -g
DEBUG = -g -fsanitize=undefined -fsanitize=address -fsanitize=leak

compile: compile.c
	gcc -std=c11 -Wall -pedantic $(DEBUG) compile.c -o compile

interpret: interpret.c
	gcc -std=c11 -Wall -pedantic $(DEBUG) interpret.c -o interpret
