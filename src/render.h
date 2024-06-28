#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>
#include "free_glyph.h"
#include "simple_renderer.h"
#include "editor.h"


extern float tokenInterpolationProgress;
extern float tokenLerpSpeed;
extern bool tokenLerp;
extern float lineNumberWidth;
extern  bool mixSelectionColor;


void update_tokens_interpolation();
void editor_render(SDL_Window *window, Free_Glyph_Atlas *atlas, Simple_Renderer *sr, Editor *editor);
void render_search_text(Free_Glyph_Atlas *minibuffer_atlas, Simple_Renderer *sr, Editor *editor);
/* void render_M_x(Free_Glyph_Atlas *atlas, Simple_Renderer *sr, Editor *editor); */
/* void render_M_x(Free_Glyph_Atlas *atlas, Simple_Renderer *sr, Editor *editor, const char *prefixText); */
void render_minibuffer_content(Free_Glyph_Atlas *atlas, Simple_Renderer *sr, Editor *editor, const char *prefixText);
void render_line_numbers(Simple_Renderer *sr, Free_Glyph_Atlas *atlas, Editor *editor);

#include "file_browser.h"
void render_markdown(Free_Glyph_Atlas *atlas, Simple_Renderer *sr, Editor *editor, File_Browser *fb);

void render_column(Simple_Renderer *sr, Free_Glyph_Atlas *atlas, Editor *editor);


// TODO lerp tokens 
typedef struct {
    Vec4f originalColor;
    Vec4f targetColor;
    float interpolationProgress;
} TokenColorData;







#endif // RENDER_H
