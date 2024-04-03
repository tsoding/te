@echo off
rem launch this from msvc-enabled console

set CFLAGS=/W4 /std:c11 /wd4996 /wd5105 /FC /TC /Zi /nologo
set INCLUDES=/I dependencies\SDL2\include /I dependencies\freetype\include /I dependencies\minirent /I dependencies\GLFW\include /I dependencies\GLEW\include
set LIBS=dependencies\SDL2\lib\x64\SDL2.lib ^
         dependencies\SDL2\lib\x64\SDL2main.lib ^
         dependencies\GLFW\lib\glfw3.lib ^
         dependencies\GLEW\lib\glew32s.lib ^
         "dependencies\freetype\release static\vs2015-2022\win64\freetype.lib" ^
         opengl32.lib User32.lib Gdi32.lib Shell32.lib

cl.exe %CFLAGS% %INCLUDES% /Feded src\main.c src\la.c src\editor.c src\file_browser.c src\free_glyph.c src\simple_renderer.c src\common.c src\lexer.c /link %LIBS% -SUBSYSTEM:windows
