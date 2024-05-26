#ifndef FILE_BROWSER_H_
#define FILE_BROWSER_H_

#include "common.h"
#include "free_glyph.h"

#include <SDL3/SDL.h>

typedef struct {
    Files files;
    size_t cursor;
    String_Builder dir_path;
    String_Builder file_path;
} File_Browser;

Errno fb_open_dir(File_Browser *fb, const char *dir_path);
Errno fb_change_dir(File_Browser *fb);
void fb_render(const File_Browser *fb, SDL_Window *window, Free_Glyph_Atlas *atlas, Simple_Renderer *sr);
const char *fb_file_path(File_Browser *fb);

#endif // FILE_BROWSER_H_
