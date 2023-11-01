# Modules and their (non-cyclic) dependencies. No header is included twice, so
# that no protection against multiple inclusion is needed. Each module is either
# a utility module used only in .c files, or else its header is included in only
# one other header. If the headers of both A and B need C's header, then A and B
# are made artificially dependent, so that A includes B's header which includes
# C's header. If lists $(x) and $(y) both include the same file, then $(sort
# $(x) $(y)) is used to make the list unique.
unicode = unicode.c
files = files.c $(unicode)
settings = settings.c $(files)

events = events.c
handler = handler.c $(events) $(unicode)
display = display.c $(handler)

test = text.c $(unicode)

store = store.c

# Allegro libraries
allegro = -lallegro -lallegro_main -lallegro_font -lallegro_ttf

text = text.c $(unicode)
tags = tags.c

# Compile flags. May need to remove sanitize flags for Windows.
FLAGS = -std=c18 -Wall -pedantic
DEBUG = -g -fsanitize=undefined -fsanitize=address

# Default rule for building and testing a module.
%: %.c
	@ clang $(DEBUG) -DTEST$@ $(FLAGS) $($@) $(allegro) -o test
	@ ASAN_OPTIONS=detect_leaks=1 \
	LSAN_OPTIONS=suppressions=leaks.txt,print_suppressions=0 \
	./test
	@ rm test

snipe: snipe.c $(display)
	clang -std=c11 -Wall snipe.c $(display) $(allegro) -o snipe -fsanitize=undefined
