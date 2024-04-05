#!/bin/sh

set -xe

CC="${CXX:-cc}"
PKGS="sdl2 glew freetype2"
CFLAGS="-std=c11 -pedantic -ggdb -Idependencies/miniconf -Idependencies/minirent -Idependencies/minifmt"
LIBS=-lm
SRC="src/main.c src/la.c src/editor.c src/file_browser.c src/free_glyph.c src/simple_renderer.c src/common.c src/lexer.c"

if [ `uname` = "Darwin" ]; then
    CFLAGS+=" -framework OpenGL"
fi

$CC $CFLAGS `pkg-config --cflags $PKGS` -g -o ded $SRC $LIBS `pkg-config --libs $PKGS`
