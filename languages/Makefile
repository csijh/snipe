# To test out a new language definition x.txt:
#    make compile
#    ./compile x.txt
#    make interpret
#    ./interpret x.bin

#DEBUG = -g
DEBUG = -g -fsanitize=undefined -fsanitize=address -fsanitize=leak

compile: src/compile.c
	gcc -std=c11 -Wall -pedantic $(DEBUG) src/compile.c -o compile

interpret: src/interpret.c
	gcc -std=c11 -Wall -pedantic $(DEBUG) src/interpret.c -o interpret
