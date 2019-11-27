# To test out a new language definition x.txt:
#    make compile
#    ./compile x.txt
#    make interpret
#    ./interpret x.bin

DEBUG = -g
#DEBUG = -g -fsanitize=undefined -fsanitize=address -fsanitize=leak

compile: compile.c
	gcc -std=c11 -Wall -pedantic $(DEBUG) compile.c -o compile

interpret: interpret.c
	gcc -std=c11 -Wall -pedantic $(DEBUG) interpret.c -o interpret
