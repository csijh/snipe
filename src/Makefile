# Compile snipe or test its modules. This file holds the definitive information
# about module dependencies.

# Find the OS platform using the uname command.
Linux := $(findstring Linux, $(shell uname -s))
MacOS := $(findstring Darwin, $(shell uname -s))
Windows := $(findstring NT, $(shell uname -s))

# Compiler options to include OpenGL and system libraries needed by GLFW,
# advanced debugging options where available, and the name of the executable,
# for the Linux platform.
ifdef Linux
	SYS = -lGL -lm -ldl -lXrandr -lXinerama -lXcursor -lpthread \
	-lXxf86vm -lX11
	DEBUG = -fsanitize=address -fsanitize=undefined
	EXEC = snipe
endif

# Same for the MacOS platform
ifdef MacOS
	MM = mmmm
	SYS = -framework Cocoa -framework IOKit -framework CoreFoundation \
	-framework CoreVideo -framework OpenGL
	DEBUG = -fsanitize=address -fsanitize=undefined
	EXEC = snipe
endif

# Same for the Windows platform
ifdef Windows
	SYS = -lopengl32 -lgdi32
	DEBUG =
	EXEC = snipe.exe
endif

# Compiler options to include Freetype, and GLFW.
FT = -I../freetype/include -L../freetype/objs/ -lfreetype
GL = -I../glfw/include -L../glfw/lib -lglfw3 $(SYS)

# Compiler commands for production compiling, and for test compiling.
# The gcc compiler ca be replaced by clang, if desired.
PCC = gcc -std=c11 -o $(EXEC) -O2 -DNDEBUG $@.c
TCC = gcc -std=c11 -o $(EXEC) -Wall -pedantic -g $(DEBUG) -Dtest_$@ $@.c

# Build a cut down version of freetype (with no dependencies) from source.
../freetype/objs/libfreetype.a:
	rm -rf ../freetype/objs
	mkdir ../freetype/objs
	cd ../freetype && make setup ansi
	cd ../freetype && make

# Build the glfw static library (with only system dependencies) from source.
../glfw/lib/libglfw3.a:
	rm -rf ../glfw/lib
	mkdir ../glfw/lib
	cd ../glfw/lib && cmake -G "Unix Makefiles" ..
	cd ../glfw/lib && make
	mv ../glfw/lib/src/libglfw3.a ../glfw
	rm -rf ../glfw/lib/*
	mv ../glfw/libglfw3.a ../glfw/lib

# The modules are listed in dependency order.
# ------------------------------------------

# The modules which are shared by the view and the model.
UTILS = action.c style.c setting.c string.c list.c array.c file.c
.PHONY: $(UTILS)

file:
	$(TCC)
	./snipe

array:
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

font: ../freetype/objs/libfreetype.a
	$(TCC) file.c $(FT)
	./snipe

event:
	$(TCC)
	./snipe

handler: ../glfw/lib/libglfw3.a
	$(TCC) event.c theme.c $(UTILS) $(GL)
	./snipe

display: ../glfw/lib/libglfw3.a
	$(TCC) handler.c event.c font.c theme.c $(UTILS) $(FT) $(GL)
	./snipe

# The modules which form the model.
MODEL = document.c text.c history.c cursor.c indent.c scan.c line.c lines.c
.PHONY: $(MODEL)

lines:
	$(TCC) array.c
	./snipe

line:
	$(TCC) list.c
	./snipe

scan:
	$(TCC) $(UTILS)
	./snipe

indent:
	$(TCC) style.c
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
	$(TCC) scan.c indent.c text.c history.c cursor.c line.c theme.c $(UTILS)
	./snipe

# The modules which form the controller: snipe is a test version of the whole
# editor, psnipe is a production version to go in the directory above, zip
# makes a binary zip file for release.
.PHONY: map snipe psnipe zip

map:
	$(TCC) $(MODEL) $(VIEW) $(UTILS) $(FT) $(GL)
	./snipe

snipe:
	cp setting.h style.h event.h action.h Makefile ../help/
	$(TCC) map.c $(MODEL) $(VIEW) $(UTILS) $(FT) $(GL)

psnipe:
	cp setting.h style.h event.h action.h Makefile ../help/
	$(PCC) map.c $(MODEL) $(VIEW) $(UTILS) $(FT) $(GL)

zip:
	mv $(EXEC) ..
	cd ../.. && zip -r snipe.zip snipe/files snipe/help snipe/languages \
	snipe/settings.txt snipe/$(EXEC)
