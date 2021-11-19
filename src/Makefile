# Modules in dependency order.
unicode = unicode.c
files = files.c $(unicode)
settings = settings.c $(files)
events = events.c $(files)
handler = handler.c $(events)
display = display.c $(handler)

# Allegro libraries
allegro = -lallegro -lallegro_main -lallegro_font -lallegro_ttf

text = text.c $(unicode)
tags = tags.c

# Compile flags. May need to remove sanitize flags for Windows.
FLAGS = -std=c11 -Wall -pedantic
DEBUG = -g -fsanitize=undefined -fsanitize=address

# Default rule for building and testing a module.
%: %.c
	@ clang $(DEBUG) -DTEST$@ $(FLAGS) $($@) $(allegro) -o test
	@ ASAN_OPTIONS=detect_leaks=1 \
	LSAN_OPTIONS=suppressions=leaks.txt,print_suppressions=0 \
	./test
	@ rm test

snipe: snipe.c display.c events.c
	clang -std=c11 -Wall snipe.c $(files) $(display) events.c -o snipe -fsanitize=undefined
