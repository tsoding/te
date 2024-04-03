#ifndef EDITOR_H_
#define EDITOR_H_

#include <stdlib.h>
#include "common.h"
#include "free_glyph.h"
#include "simple_renderer.h"
#include "lexer.h"

#include <SDL2/SDL.h>

typedef struct {
    size_t begin;
    size_t end;
} Line;

typedef struct {
    Line *items;
    size_t count;
    size_t capacity;
} Lines;

typedef struct {
    Token *items;
    size_t count;
    size_t capacity;
} Tokens;

typedef struct {
    Uint32 uid;
    const char *msg;
    size_t msg_size;
    Vec4f color;
    Uint32 when;
    Uint32 lasts;
} PopUp;

typedef struct {
    PopUp *items;
    size_t count;
} PopUps;

struct Editor_s;

typedef struct {
    String_Builder text;
    bool active;
    bool required;
    void (*onDone)(struct Editor_s *);
} Input;

typedef struct Editor_s {
    Free_Glyph_Atlas *atlas;

    String_Builder data;
    Lines lines;
    Tokens tokens;
    String_Builder file_path;
    PopUps popUps;
    Input input;

    bool searching;

    bool selection;
    size_t select_begin;
    size_t cursor;

    Uint32 last_stroke;

    String_Builder clipboard;
} Editor;

Errno editor_save_as(Editor *editor, const char *file_path);
Errno editor_save(Editor *editor);
Errno editor_load_from_file(Editor *editor, const char *file_path);

Uint32 editor_add_popup(Editor *editor, PopUp *popUp);
void editor_remove_popup(Editor *editor, Uint32 uid);

void editor_start_input(Editor *editor);

void editor_backspace(Editor *editor);
void editor_delete(Editor *editor);
void editor_delete_selection(Editor *editor);
size_t editor_cursor_row(const Editor *e);

void editor_move_line_up(Editor *e);
void editor_move_line_down(Editor *e);
void editor_move_char_left(Editor *e);
void editor_move_char_right(Editor *e);
void editor_move_word_left(Editor *e);
void editor_move_word_right(Editor *e);

void editor_move_to_begin(Editor *e);
void editor_move_to_end(Editor *e);
void editor_move_to_line_begin(Editor *e);
void editor_move_to_line_end(Editor *e);

void editor_move_paragraph_up(Editor *e);
void editor_move_paragraph_down(Editor *e);

void editor_insert_char(Editor *e, char x);
void editor_insert_buf(Editor *e, char *buf, size_t buf_len);
void editor_retokenize(Editor *e);
void editor_render(SDL_Window *window, Free_Glyph_Atlas *atlas, Simple_Renderer *sr, Editor *editor);
void editor_update_selection(Editor *e, bool shift);
void editor_clipboard_copy(Editor *e);
void editor_clipboard_paste(Editor *e);
void editor_start_search(Editor *e);
void editor_stop_search(Editor *e);
bool editor_search_matches_at(Editor *e, size_t pos);

void flash_error_str(Editor *editor, const char *str);

// TODO: display errors reported via flash_error right in the text editor window somehow
#define flash_error(editor, ...) do { static char buf[200]; sprintf(buf, __VA_ARGS__); flash_error_str(editor, buf); } while(0)

#endif // EDITOR_H_
