# Compile snipe or test its modules.

# This is the definitive list of modules and their dependencies.
# First, the utility modules which are shared by the view and the model.
unicode = unicode.c
list = list.c
file = file.c list.c unicode.c
array = array.c
string = string.c list.c
setting = setting.c string.c $(file)
style = style.c string.c $(file)
action = action.c string.c $(file)
UTILS = action.c style.c setting.c string.c array.c $(file)

# The modules which form the model.
# change is a proposed new module.
# gap is a proposed replacement for the text module.
# lines is not currently used.
change = change.c
gap = gap.c string.c $(file)
lines = lines.c array.c
line = line.c list.c
scan = scan.c $(UTILS)
indent = indent.c style.c
cursor = cursor.c style.c line.c list.c
history = history.c list.c
text = text.c cursor.c style.c line.c string.c $(file)
document = $(MODEL) $(UTILS)
MODEL = document.c text.c history.c cursor.c indent.c scan.c line.c

# The modules which form the view.
theme = theme.c style.c setting.c string.c $(file)
font = font.c $(file) $(FT)
event = event.c
queue = queue.c event.c
handler = handler.c queue.c event.c theme.c $(UTILS) $(GL)
display = $(VIEW) $(UTILS) $(FT) $(GL)
VIEW = display.c handler.c font.c queue.c event.c theme.c

# The modules which form the controller.
map = map.c $(MODEL) $(VIEW) $(UTILS) $(FT) $(GL)
snipe = snipe.c $(map)

# Default rule for building and testing a module. Insist on the FT and GLFW
# libraries being built first. (Use gcc or clang.)
%: %.c $(LIBFT) $(LIBGL)
	gcc -std=c11 -o snipe -Wall -pedantic -g $(DEBUG) -D$@Test $($@)
	./snipe

# Copy files into the help folder, so that help pages can refer to them.
HELPFILES:
	cp setting.h style.h event.h action.h Makefile ../help/

# For testing snipe as a whole, add HELPFILES as an extra dependency.
snipe: HELPFILES

# Type make psnipe for a production version. (Use gcc or clang.)
psnipe: $(LIBFT) $(LIBGL) HELPFILES
	gcc -std=c11 -o snipe -O2 -DNDEBUG $($@)

# Make a zip file for distribution. Assumes a production version of snipe has
# been copied to the directory above src. Change to snipe.exe for Windows.
zip:
	cd ../.. && zip -r snipe.zip snipe/files snipe/help snipe/languages \
	snipe/settings.txt snipe/snipe

# Advanced debugging options (not easily available on Windows)
ifdef Windows
	DEBUG =
else
	DEBUG = -fsanitize=address -fsanitize=undefined
endif

# ---------- FT and GLFW libraries ----------

# Find the OS platform using the uname command.
Linux := $(findstring Linux, $(shell uname -s))
MacOS := $(findstring Darwin, $(shell uname -s))
Windows := $(findstring NT, $(shell uname -s))

# Define system libraries needed by GLFW (different on each platform). Force
# pthread to be linked statically on Windows.
ifdef Linux
	SYS = -lGL -lm -ldl -lXrandr -lXinerama -lXcursor -lpthread -lXxf86vm -lX11
else ifdef MacOS
	SYS = -framework Cocoa -framework IOKit -framework CoreFoundation \
	-framework CoreVideo -framework OpenGL
else ifdef Windows
	SYS = -lopengl32 -lgdi32 -Wl,-Bstatic -lpthread -Wl,-Bdynamic
endif

# The LIB variables are the library files for freetype and GLFW. Use explicit
# paths, not -l, to ensure the right versions get linked. The FT and GL
# variables add the headers.
LIBFT = ../freetype/objs/libfreetype.a
LIBGL = ../glfw/lib/libglfw3.a
FT = -I../freetype/include $(LIBFT)
GL = -I../glfw/include $(LIBGL) $(SYS)

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
