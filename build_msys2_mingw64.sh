#!/bin/sh

set -xe

PKGS="--static sdl2 glew freetype2"
CFLAGS="-Wall -Wextra -pedantic -ggdb -DGLEW_STATIC `pkg-config --cflags $PKGS` -Isrc -Idependencies/miniconf -Idependencies/minirent -Dassert(expression)=((void)0) "
LIBS="-lm -lopengl32 `pkg-config --libs $PKGS`"
SRC="src/main.c src/la.c src/editor.c src/file_browser.c src/free_glyph.c src/simple_renderer.c src/common.c src/lexer.c""
OBJ=$(echo "$SRC" | sed "s/\.c/\.o/g")
OBJ=$(echo "$OBJ" | sed "s/src\// /g")

# wget "https://raw.githubusercontent.com/tsoding/minirent/master/minirent.h" -P /src
gcc -std=c11 $CFLAGS -c $SRC
# some libs linked with c++ stuff
g++ -o life.exe $OBJ $LIBS $LIBS -static

