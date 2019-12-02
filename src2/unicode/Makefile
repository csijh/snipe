# To reconstruct or upgrade the module in ../unicode.h, ../unicode.c:
# make and run unigen, then test unicode in the folder above.

# Find the OS platform using the uname command.
Linux := $(findstring Linux, $(shell uname -s))
MacOS := $(findstring Darwin, $(shell uname -s))
Windows := $(findstring NT, $(shell uname -s))

DATA = UnicodeData.txt GraphemeBreakProperty.txt emoji-data.txt
TESTS = GraphemeBreakTest.txt
DEBUG = -g -fsanitize=undefined -fsanitize=address
ifdef Windows
	DEBUG = -g
endif

unigen: unigen.c $(DATA) $(TESTS)
	gcc -std=c11 -Wall $(DEBUG) unigen.c -o unigen
