# Modules forming the editor's model.
unicode = unicode.c
text = text.c $(unicode)
history = history.c $(text)


string = string.c
cursors = cursors.c


edit = edit.c $(unicode)
lines = lines.c $(edit)
tokens = tokens.c

array = array.c

scan = scan.c $(cursors)
brackets = brackets.c $(scan)
indent = indent.c $(brackets)
document = document.c $(indent)
model = $(document)

# Find the OS platform using the uname command.
Linux := $(findstring Linux, $(shell uname -s))
MacOS := $(findstring Darwin, $(shell uname -s))
Windows := $(findstring NT, $(shell uname -s))

FLAGS = -std=c11 -Wall -pedantic
DEBUG = -g -fsanitize=undefined -fsanitize=address
ifdef Windows
	DEBUG = -g
endif
OPT = -O2 -flto

# Default rule for building and testing a module.
%: %.c
	gcc $(DEBUG) -D$@Test $(FLAGS) $($@) -o test
	./test
	rm test

# Build the whole model, as a library.
model:
	gcc $(OPT) $(FLAGS) -c $($@)
	ar rcs model.a *.o
	rm *.o
