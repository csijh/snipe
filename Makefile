# Compile snipe or test its modules. This file holds the definitive information
# about module dependencies.

# Find the OS platform using the uname command.
OS = $(shell uname -s)

# Compiler options to include OpenGL and other system libraries, for the
# Linux/MacOS/Windows platforms.
ifeq ($(OS), Linux)
	SYS = -DLINUX -lGL `pkg-config --static --libs glfw3`
else ifeq ($(OS), Darwin)
	SYS = -DMACOS -lGL `pkg-config --static --libs glfw3`
else ifeq ($(OS), MINGW64_NT-6.1)
	SYS = -DWINDOWS -lopengl32 -lgdi32
endif

# Advanced debugging options (not easily available on Windows).
ifeq ($(OS), MINGW64_NT-6.1)
	DEBUG =
else
	DEBUG = -fsanitize=address -fsanitize=undefined
endif

# Compiler options to include Freetype, and GLFW.
FT = -Ifreetype/include -Lfreetype/objs/ -lfreetype
GL = -Iglfw/include -Lglfw/lib -lglfw3 $(SYS)
LIBS = $(FT) $(GL)

# Compiler commands for production compiling, and for test compiling.
PCC = gcc -std=c11 -o snipe -O2 $@.c
TCC = gcc -std=c11 -o snipe -Wall -pedantic -g $(DEBUG) -Dtest_$@ $@.c

# Build the glfw static library (with only system dependencies) from source.
glfw/lib/libglfw3.a:
	rm -rf glfw/lib
	mkdir glfw/lib
	cd glfw/lib && cmake -G "Unix Makefiles" ..
	cd glfw/lib && make
	mv glfw/lib/src/libglfw3.a glfw
	rm -rf glfw/lib/*
	mv glfw/libglfw3.a glfw/lib

# Build a cut down version of freetype (with no dependencies) from source.
freetype/objs/libfreetype.a:
	rm -rf freetype/objs
	mkdir freetype/objs
	cd freetype && make setup ansi
	cd freetype && make

# The modules are listed in dependency order.

# The modules which are shared by the view and the model.
UTILS = action.c style.c setting.c string.c list.c file.c
.PHONY: $(UTILS)

file:
	$(TCC)
	./snipe

list:
	$(TCC)
	./snipe

string:
	$(TCC) list.c
	./snipe

setting:
	$(TCC) string.c list.c file.c $(SYS)
	./snipe

style:
	$(TCC) string.c list.c file.c
	./snipe

action:
	$(TCC) string.c list.c file.c
	./snipe

# The modules which form the view.
VIEW = display.c handler.c font.c event.c theme.c
.PHONY: $(VIEW)

theme:
	$(TCC) style.c setting.c string.c list.c file.c
	./snipe

font: freetype/objs/libfreetype.a
	$(TCC) file.c $(FT)
	./snipe

event:
	$(TCC)
	./snipe

handler:
	$(TCC) event.c theme.c $(UTILS) $(GL)
	./snipe

display:
	$(TCC) handler.c event.c font.c theme.c $(UTILS) $(GL) $(FT)
	./snipe

# The modules which form the model.
MODEL = document.c text.c history.c cursor.c line.c scan.c
.PHONY: $(MODEL)

scan:
	$(TCC) $(UTILS)
	./snipe

line:
	$(TCC) list.c
	./snipe

cursor:
	$(TCC) style.c line.c list.c
	./snipe

history:
	$(TCC) list.c
	./snipe

text:
	$(TCC) cursor.c style.c line.c string.c list.c file.c
	./snipe

document:
	$(TCC) scan.c text.c history.c cursor.c line.c theme.c $(UTILS)
	./snipe

# The modules which form the controller.
.PHONY: map snipe

map:
	$(TCC) $(MODEL) $(VIEW) $(UTILS) $(LIBS)
	./snipe

snipe:
	cp setting.h style.h event.h action.h Makefile help/
	$(TCC) map.c $(MODEL) $(VIEW) $(UTILS) $(LIBS)
