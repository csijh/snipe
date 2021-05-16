unicode = unicode.c
allegro = -lallegro -lallegro_main -lallegro_font -lallegro_ttf
display = display.c $(allegro) $(unicode)
events = events.c $(allegro)

files = files.c
settings = settings.c $(files) $(allegro)
text = text.c $(unicode)
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
	clang -std=c11 -Wall snipe.c $(files) $(display) events.c -o snipe -fsanitize=undefined
