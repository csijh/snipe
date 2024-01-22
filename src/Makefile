# The modules and their dependencies.
array = array.c

unicode = unicode.c
file = file.c unicode.c array.c check.c
kinds = kinds.c
text = text.c kinds.c $(file)
brackets = brackets.c text.c kinds.c
lines = lines.c text.c kinds.c
event = event.c
handler = handler.c event.c unicode.c check.c
display = display.c kinds.c array.c $(handler)

# Allegro libraries (-lallegro_main needed for OSX)
allegro = -lallegro -lallegro_main -lallegro_font -lallegro_ttf

# Debugging options (remove if not available)
DEBUG = -fsanitize=address -fsanitize=undefined

# Runtime suppression of memory leak messages for Linux system libraries.
RUN = ASAN_OPTIONS=detect_leaks=1 \
LSAN_OPTIONS=suppressions=leaks.txt,print_suppressions=0

# Default rule for building and testing a module.
# @ means silent; use make -n to see the generated commands.
%: %.c
	@gcc -std=c11 -o run -Wall -pedantic -g $(DEBUG) -D$@Test $($@) $(allegro)
	@$(RUN) ./run
