@echo off
rem launch this from msvc-enabled console

set CFLAGS=/W4 /WX /std:c11 /wd4996 /wd5105 /FC /TC /Zi /nologo
set INCLUDES=/I dependencies\SDL3\include /I dependencies\GLFW\include /I dependencies\GLEW\include
set LIBS=dependencies\SDL3\lib\x64\SDL3.lib ^
         dependencies\SDL3\lib\x64\SDL3main.lib ^
         dependencies\GLFW\lib\glfw3.lib ^
         dependencies\GLEW\lib\glew32s.lib ^
         opengl32.lib User32.lib Gdi32.lib Shell32.lib

cl.exe %CFLAGS% %INCLUDES% /Feded src\main.c src\la.c src\editor.c src\file_browser.c src\free_glyph.c src\simple_renderer.c src\common.c /link %LIBS% -SUBSYSTEM:windows
