#include <assert.h>
#include "editor.h"
#include "theme.h"
#include <stdbool.h>
#include "./free_glyph.h"

void free_glyph_atlas_init(Free_Glyph_Atlas *atlas, FT_Face face)
{
    // TODO: Introduction of SDF font slowed down the start up time
    // We need to investigate what's up with that
    FT_Int32 load_flags = FT_LOAD_RENDER | FT_LOAD_TARGET_(FT_RENDER_MODE_SDF);
    for (int i = 32; i < 128; ++i) {
        if (FT_Load_Char(face, i, load_flags)) {
            fprintf(stderr, "ERROR: could not load glyph of a character with code %d\n", i);
            exit(1);
        }

        atlas->atlas_width += face->glyph->bitmap.width;
        if (atlas->atlas_height < face->glyph->bitmap.rows) {
            atlas->atlas_height = face->glyph->bitmap.rows;
        }
    }

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &atlas->glyphs_texture);
    glBindTexture(GL_TEXTURE_2D, atlas->glyphs_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        (GLsizei) atlas->atlas_width,
        (GLsizei) atlas->atlas_height,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        NULL);

    int x = 0;
    for (int i = 32; i < 128; ++i) {
        if (FT_Load_Char(face, i, load_flags)) {
            fprintf(stderr, "ERROR: could not load glyph of a character with code %d\n", i);
            exit(1);
        }

        if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
            fprintf(stderr, "ERROR: could not render glyph of a character with code %d\n", i);
            exit(1);
        }

        atlas->metrics[i].ax = face->glyph->advance.x >> 6;
        atlas->metrics[i].ay = face->glyph->advance.y >> 6;
        atlas->metrics[i].bw = face->glyph->bitmap.width;
        atlas->metrics[i].bh = face->glyph->bitmap.rows;
        atlas->metrics[i].bl = face->glyph->bitmap_left;
        atlas->metrics[i].bt = face->glyph->bitmap_top;
        atlas->metrics[i].tx = (float) x / (float) atlas->atlas_width;

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            x,
            0,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer);
        x += face->glyph->bitmap.width;
    }
}

float free_glyph_atlas_cursor_pos(const Free_Glyph_Atlas *atlas, const char *text, size_t text_size, Vec2f pos, size_t col)
{
    for (size_t i = 0; i < text_size; ++i) {
        if (i == col) {
            return pos.x;
        }

        size_t glyph_index = text[i];
        if (glyph_index >= GLYPH_METRICS_CAPACITY) {
            glyph_index = '?';
        }

        Glyph_Metric metric = atlas->metrics[glyph_index];
        pos.x += metric.ax;
        pos.y += metric.ay;
    }

    return pos.x;
}

void free_glyph_atlas_measure_line_sized(Free_Glyph_Atlas *atlas, const char *text, size_t text_size, Vec2f *pos) {
    for (size_t i = 0; i < text_size; ++i) {
        size_t glyph_index = text[i];
        // TODO: support for glyphs outside of ASCII range
        if (glyph_index >= GLYPH_METRICS_CAPACITY) {
            glyph_index = '?';
        }
        Glyph_Metric metric = atlas->metrics[glyph_index];

        pos->x += metric.ax;
        pos->y += metric.ay;
    }
}


// ALMOST TODO multiple lines
void free_glyph_atlas_render_line_sized(Free_Glyph_Atlas *atlas,
                                        Simple_Renderer *sr, const char *text,
                                        size_t text_size, Vec2f *pos,
                                        Vec4f color) {
  // Determine the line and character position of the cursor using the global
  // editor
  size_t cursor_line = 0;
  size_t cursor_character = 0;

  // Find the line that contains the cursor
  for (size_t row = 0; row < editor.lines.count; ++row) {
    Line line = editor.lines.items[row];
    if (line.begin <= editor.cursor && editor.cursor <= line.end) {
      cursor_line = row;
      cursor_character = editor.cursor - line.begin;
      break;
    }
  }

  size_t current_pos = 0;

  for (size_t i = 0; i < text_size; ++i) {
    size_t glyph_index = text[i];
    if (glyph_index >= GLYPH_METRICS_CAPACITY) {
      glyph_index = '?'; // Fallback for unsupported glyphs
    }
    Glyph_Metric metric = atlas->metrics[glyph_index];
    float x2 = pos->x + metric.bl;
    float y2 = -pos->y - metric.bt;
    float w = metric.bw;
    float h = metric.bh;

    // Use the background color if this character is at the cursor position and
    // on the correct line
    Vec4f use_color = color;
    if (cursor_line * FREE_GLYPH_FONT_SIZE <= pos->y &&
        pos->y < (cursor_line + 1) * FREE_GLYPH_FONT_SIZE &&
        current_pos == cursor_character) {
      use_color = CURRENT_THEME.background;
    }

    simple_renderer_image_rect(sr, vec2f(x2, -y2), vec2f(w, -h),
                               vec2f(metric.tx, 0.0f),
                               vec2f(metric.bw / (float)atlas->atlas_width,
                                     metric.bh / (float)atlas->atlas_height),
                               use_color);

    // Advance the cursor after rendering the glyph
    pos->x += metric.ax;
    pos->y += metric.ay;

    // Handle newlines
    if (text[i] == '\n') {
      pos->y -= FREE_GLYPH_FONT_SIZE;
      pos->x = 0;
    }

    current_pos++;
  }
}


/* ORIGINAL */
/* void free_glyph_atlas_render_line_sized(Free_Glyph_Atlas *atlas, Simple_Renderer *sr, const char *text, size_t text_size, Vec2f *pos, Vec4f color) */
/* { */
/*     for (size_t i = 0; i < text_size; ++i) { */
/*         size_t glyph_index = text[i]; */
/*         // TODO: support for glyphs outside of ASCII range */
/*         if (glyph_index >= GLYPH_METRICS_CAPACITY) { */
/*             glyph_index = '?'; */
/*         } */
/*         Glyph_Metric metric = atlas->metrics[glyph_index]; */
/*         float x2 = pos->x + metric.bl; */
/*         float y2 = -pos->y - metric.bt; */
/*         float w  = metric.bw; */
/*         float h  = metric.bh; */

/*         pos->x += metric.ax; */
/*         pos->y += metric.ay; */

/*         simple_renderer_image_rect( */
/*             sr, */
/*             vec2f(x2, -y2), */
/*             vec2f(w, -h), */
/*             vec2f(metric.tx, 0.0f), */
/*             vec2f(metric.bw / (float) atlas->atlas_width, metric.bh / (float) atlas->atlas_height), */
/*             color); */
/*     } */
/* } */


// ADDED

float free_glyph_atlas_measure_line_width(Free_Glyph_Atlas *atlas, const char *text, size_t text_size) {
    Vec2f pos = {0.0f, 0.0f};
    for (size_t i = 0; i < text_size; ++i) {
        size_t glyph_index = text[i];
        // TODO: support for glyphs outside of ASCII range
        if (glyph_index >= GLYPH_METRICS_CAPACITY) {
            glyph_index = '?';
        }
        Glyph_Metric metric = atlas->metrics[glyph_index];
        pos.x += metric.ax;
    }
    return pos.x;
}
