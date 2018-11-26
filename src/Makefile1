# Compile snipe or test its modules. This file holds the definitive information
# about module dependencies.

# Find the OS platform using the uname command.
Linux := $(findstring Linux, $(shell uname -s))
MacOS := $(findstring Darwin, $(shell uname -s))
Windows := $(findstring NT, $(shell uname -s))

# The name of the executable (clang on Windows doesn't add .exe automatically).
ifdef Windows
	EXEC = snipe.exe
else
	EXEC = snipe
endif

# Advanced debugging options (not easily available on Windows)
ifdef Windows
	DEBUG =
else
	DEBUG = -fsanitize=address -fsanitize=undefined
endif

# System libraries needed by GLFW (different on each platform). Force
# pthread to be linked statically on Windows.
ifdef Linux
	SYS = -lGL -lm -ldl -lXrandr -lXinerama -lXcursor -lpthread -lXxf86vm -lX11
else ifdef MacOS
	SYS = -framework Cocoa -framework IOKit -framework CoreFoundation \
	-framework CoreVideo -framework OpenGL
else ifdef Windows
	SYS = -lopengl32 -lgdi32 $(FT) -Wl,-Bstatic -lpthread -Wl,-Bdynamic
endif

# The library files for freetype and GLFW, and their headers. Use explicit
# paths for the libraries, not -l, to ensure linking to the right version.
LIBFT = ../freetype/objs/libfreetype.a
LIBGL = ../glfw/lib/libglfw3.a
FT = -I../freetype/include $(LIBFT)
GL = -I../glfw/include $(LIBGL) $(SYS)

# Compiler commands for production compiling, and for test compiling.
# The gcc compiler can be replaced by clang, if desired.
PCC = gcc -std=c11 -o $(EXEC) -O2 -DNDEBUG
TCC = gcc -std=c11 -o $(EXEC) -Wall -pedantic -g $(DEBUG) -Dtest_$@

# Build a cut down version of freetype (with no dependencies) from source.
$(LIBFT):
	rm -rf ../freetype/objs
	mkdir ../freetype/objs
	cd ../freetype && make setup ansi
	cd ../freetype && make

# Build the glfw static library (with only system dependencies) from source.
$(LIBGL):
	rm -rf ../glfw/lib
	mkdir ../glfw/lib
	cd ../glfw/lib && cmake -G "Unix Makefiles" ..
	cd ../glfw/lib && make
	mv ../glfw/lib/src/libglfw3.a ../glfw
	rm -rf ../glfw/lib/*
	mv ../glfw/libglfw3.a ../glfw/lib

# The modules, listed in dependency order.
# ----------------------------------------

# The modules which are shared by the view and the model.
UTILS = action.c style.c setting.c string.c array.c file.c list.c unicode.c
.PHONY: $(UTILS)

unicode:
	$(TCC) unicode.c
	./snipe

list:
	$(TCC) list.c
	./snipe

file:
	$(TCC) file.c list.c unicode.c
	./snipe

array:
	$(TCC) array.c
	./snipe

string:
	$(TCC) string.c list.c
	./snipe

setting:
	$(TCC) setting.c string.c list.c file.c
	./snipe

style:
	$(TCC) style.c string.c list.c file.c
	./snipe

action:
	$(TCC) action.c string.c list.c file.c
	./snipe

# The modules which form the view.
VIEW = display.c handler.c font.c queue.c event.c theme.c
.PHONY: $(VIEW)

theme:
	$(TCC) theme.c style.c setting.c string.c list.c file.c
	./snipe

font: $(LIBFT)
	$(TCC) font.c file.c $(FT)
	./snipe

event:
	$(TCC) event.c
	./snipe

queue:
	$(TCC) queue.c event.c
	./snipe

handler: $(LIBGL)
	$(TCC) handler.c queue.c event.c theme.c $(UTILS) $(GL)
	./snipe

display: $(LIBFT) $(LIBGL)
	$(TCC) $(VIEW) $(UTILS) $(FT) $(GL)
	./snipe

# The modules which form the model.
MODEL = document.c text.c history.c cursor.c indent.c scan.c line.c lines.c \
change.c
.PHONY: $(MODEL)

change:
	$(TCC) change.c
	./snipe

lines:
	$(TCC) lines.c array.c
	./snipe

line:
	$(TCC) line.c list.c
	./snipe

scan:
	$(TCC) scan.c $(UTILS)
	./snipe

indent:
	$(TCC) indent.c style.c
	./snipe

cursor:
	$(TCC) cursor.c style.c line.c list.c
	./snipe

history:
	$(TCC) history.c list.c
	./snipe

text:
	$(TCC) text.c cursor.c style.c line.c string.c list.c file.c unicode.c
	./snipe

document:
	$(TCC) $(MODEL) $(UTILS)
	./snipe

# The modules which form the controller: snipe is a test version of the whole
# editor, psnipe is a production version to go in the directory above, zip
# makes a binary zip file for release.
.PHONY: map snipe psnipe zip

map: $(LIBFT) $(LIBGL)
	$(TCC) map.c $(MODEL) $(VIEW) $(UTILS) $(FT) $(GL)
	./snipe

snipe: $(LIBFT) $(LIBGL)
	cp setting.h style.h event.h action.h Makefile ../help/
	$(TCC) snipe.c map.c $(MODEL) $(VIEW) $(UTILS) $(FT) $(GL)

psnipe: $(LIBFT) $(LIBGL)
	cp setting.h style.h event.h action.h Makefile ../help/
	$(PCC) snipe.c map.c $(MODEL) $(VIEW) $(UTILS) $(FT) $(GL)
	cp $(EXEC) ..

# Make a binary distribution after making psnipe.
zip:
	cd ../.. && zip -r snipe.zip snipe/files snipe/help snipe/languages \
	snipe/settings.txt snipe/$(EXEC)
