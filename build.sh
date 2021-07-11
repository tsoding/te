#!/bin/sh

set -xe

CC="${CXX:-cc}"
PKGS="$1 sdl2 glew freetype2"
CFLAGS="-Wall -Wextra -std=c11 -pedantic -ggdb"
LIBS="-lm $2"
SRC="src/main.c src/la.c src/editor.c src/sdl_extra.c src/file.c src/gl_extra.c src/tile_glyph.c src/free_glyph.c src/cursor_renderer.c"

if [ `uname` = "Darwin" ]; then
    CFLAGS+=" -framework OpenGL"
fi

$CC $CFLAGS `pkg-config --cflags $PKGS` -o ded $SRC $LIBS `pkg-config --libs $PKGS` $LIBS `pkg-config --libs $PKGS`
