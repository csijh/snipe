allegro = -lallegro -lallegro_main -lallegro_font -lallegro_ttf -lallegro_primitives
display = display.c $(allegro)
events = events.c $(allegro)

files = files.c
tags = tags.c

# Compile flags. May need to remove sanitize flags for Windows.
FLAGS = -std=c11 -Wall -pedantic
DEBUG = -g -fsanitize=undefined -fsanitize=address

# Default rule for building and testing a module.
%: %.c
	clang $(DEBUG) -D$@Test $(FLAGS) $($@) -o test
	LSAN_OPTIONS=suppressions=leaks.txt ./test
	rm test

snipe: snipe.c display.c events.c
	clang -std=c11 -Wall snipe.c $(files) display.c events.c $(allegro) -o snipe -fsanitize=undefined
