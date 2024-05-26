@echo off

curl -fsSL -o SDL3-devel-2.0-VC.zip https://www.libsdl.org/release/SDL3-devel-3.0-VC.zip
tar -xf SDL3-devel-2.0-VC.zip
if not exist dependencies\ mkdir dependencies\
move SDL3-3.0 dependencies\SDL3
del SDL3-devel-3.0-VC.zip
if not exist dependencies\SDL3\temp\ mkdir dependencies\SDL3\temp\
move dependencies\SDL3\include dependencies\SDL3\temp\SDL3
move dependencies\SDL3\temp dependencies\SDL3\include

curl -fsSL -o glfw-3.3.2.bin.WIN64.zip https://github.com/glfw/glfw/releases/download/3.3.2/glfw-3.3.2.bin.WIN64.zip
tar -xf glfw-3.3.2.bin.WIN64.zip
if not exist dependencies\GLFW\lib\ mkdir dependencies\GLFW\lib\
move glfw-3.3.2.bin.WIN64\lib-vc2019\glfw3.lib dependencies\GLFW\lib\glfw3.lib
if not exist dependencies\GLFW\include\GLFW mkdir dependencies\GLFW\include\GLFW
move glfw-3.3.2.bin.WIN64\include\GLFW\glfw3.h dependencies\GLFW\include\GLFW\glfw3.h
del glfw-3.3.2.bin.WIN64.zip
rmdir /s /q glfw-3.3.2.bin.WIN64

curl -fsSL -o glew-2.1.0-win32.zip https://sourceforge.net/projects/glew/files/glew/2.1.0/glew-2.1.0-win32.zip/download
tar -xf glew-2.1.0-win32.zip
if not exist dependencies\GLEW\lib\ mkdir dependencies\GLEW\lib\
move glew-2.1.0\lib\Release\x64\glew32s.lib dependencies\GLEW\lib\glew32s.lib
if not exist dependencies\GLEW\include\GL\ mkdir dependencies\GLEW\include\GL\
move glew-2.1.0\include\GL\glew.h dependencies\GLEW\include\GL\glew.h
del glew-2.1.0-win32.zip
rmdir /s /q glew-2.1.0
