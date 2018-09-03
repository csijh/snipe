# Compile snipe or test its modules. This file holds the definitive information
# about module dependencies.

# Find the OS platform using the uname command.
Linux := $(findstring Linux, $(shell uname -s))
MacOS := $(findstring Darwin, $(shell uname -s))
Windows := $(findstring NT, $(shell uname -s))

# The static library files for freetype and GLFW
LIBFT = ../freetype/objs/libfreetype.a
LIBGL = ../glfw/lib/libglfw3.a

# Compiler options to include Freetype, and GLFW. Use explicit paths for the
# library files, not -l, to ensure linking to the right version. 
FT = -I../freetype/include $(LIBFT)
GL = -I../glfw/include $(LIBGL) $(SYS)

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

# Compiler commands for production compiling, and for test compiling.
# The gcc compiler ca be replaced by clang, if desired.
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

# The modules are listed in dependency order.
# ------------------------------------------

# The modules which are shared by the view and the model.
UTILS = action.c style.c setting.c string.c list.c array.c file.c
.PHONY: $(UTILS)

file:
	$(TCC) file.c
	./snipe

array:
	$(TCC) array.c
	./snipe

list:
	$(TCC) list.c
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
MODEL = document.c text.c history.c cursor.c indent.c scan.c line.c lines.c
.PHONY: $(MODEL)

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
	$(TCC) text.c cursor.c style.c line.c string.c list.c file.c
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
