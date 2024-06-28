#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include "./editor.h"
#include "./common.h"
/* #include "./free_glyph.h" */
#include "./file_browser.h"
#include "emacs.h"
#include "lexer.h"
#include "simple_renderer.h"
#include <ctype.h> // For isalnum
#include "evil.h"
#include "theme.h"
#include "M-x.h"
#include "utilities.h"


// TODO attach documentation to each variable
// and make an interface to modify variables at runtime à la emacs

// TODO show lock logo in the modeline when the file is in read only mode

Editor editor = {0};

bool quit = false;
float zoom_factor = 3.0f;
float min_zoom_factor = 1.0;
float max_zoom_factor = 100.0;

bool isWave = false;
size_t indentation = 4;

bool showLineNumbers = false;
bool showLineNumbersBackground = false;
bool highlightCurrentLineNumber = true;
bool relativeLineNumbers = false;

bool showWhitespaces = false;
bool copiedLine = false;
bool matchParenthesis = true;

bool zoomInInsertMode = false;
bool instantCamera = false;
bool followCursor = true;
bool centeredText = true;


bool hl_line = false;
bool showIndentationLines = true;

bool showMinibuffer = true;
bool showModeline = true;
float minibufferHeight = 21.0f;
float modelineHeight = 35.0f;
float modelineAccentWidth = 5.0f;
bool fzy = false;
bool M_x_active = false;
bool evil_command_active = false;

bool BlockInsertCursor = true;
bool highlightCurrentLineNumberOnInsertMode = true; // the loong way

bool helix = false;
bool emacs = false;
bool automatic_zoom = true;

float fringeWidth = 8.0f;
bool showFringe = true;

size_t fillColumn = 80;
float fillColumnThickness = 5.0; // if 0, it default to the width of one character
bool smartFillColumn = true;
bool showFillColumn = true;

bool readonly = false; // TODO actually use this,
//like don't save if its readonly and show a lock in the modeline

bool electric_mode = true; // whether to indent automatically when typing ";"" or "}"

// When enabled, typing an open parenthesis automatically inserts the corresponding
// closing parenthesis, and vice versa.  (Likewise for brackets, etc.).
// If the region is active, the parentheses (brackets, etc.) are
// inserted around the region instead. TODO
bool electric_pair_mode = true;

// When Delete Selection mode is enabled, typed text replaces the selection
// if the selection is active.  Otherwise, typed text is just inserted at
// point regardless of any selection.
bool delete_selection_mode = true;

// How many lines a file should have
// to be considered a long file
size_t long_file_lines = 100;

bool show_line_numbers_opening_long_files = true;
bool decenter_text_opening_long_files = true;

bool hide_line_numbers_opening_small_files = true;
bool center_text_opening_small_files = true;

bool diredfl_mode = true;



bool ctrl_x_pressed = false;

void reset_keychords() {
    ctrl_x_pressed = false;
}

void set_current_mode() {
    if (emacs) {
        current_mode = EMACS;
    } else if (helix) {
        current_mode = HELIX;
    } else {
        current_mode = NORMAL;
    }
}

EvilMode current_mode = NORMAL;


void move_camera(Simple_Renderer *sr, const char* direction, float amount) {
    if(strcmp(direction, "up") == 0) {
        sr->camera_pos.y -= amount;
    } else if(strcmp(direction, "down") == 0) {
        sr->camera_pos.y += amount;
    } else if(strcmp(direction, "left") == 0) {
        sr->camera_pos.x -= amount;
    } else if(strcmp(direction, "right") == 0) {
        sr->camera_pos.x += amount;
    } else {
        printf("Invalid direction '%s'\n", direction);
    }
}

// TODO if we are on a multiple of indentation delete the correct number of indentations
void editor_backspace(Editor *e) {
    // If in search mode, reduce the search query length
    if (e->searching) {
        if (e->search.count > 0) {
            e->search.count -= 1;
        }
    } else if (e->minibuffer_active) {
        if (e->minibuffer_text.count > 0) {
            e->minibuffer_text.count -= 1;
        }
    } else {
        // Check if the cursor is at the beginning or at the beginning of a line
        if (e->cursor == 0) return; // Cursor at the beginning, nothing to delete

        size_t cursor_pos = e->cursor;
        size_t row = editor_cursor_row(e);

        if (cursor_pos > e->data.count) {
            cursor_pos = e->data.count;
        }

        // Determine the characters before and after the cursor
        char char_before_cursor = (cursor_pos > 0) ? e->data.items[cursor_pos - 1] : '\0';
        char char_after_cursor = (cursor_pos < e->data.count) ? e->data.items[cursor_pos] : '\0';

        // Smart parentheses: delete both characters if they match
        if ((char_before_cursor == '(' && char_after_cursor == ')') ||
            (char_before_cursor == '[' && char_after_cursor == ']') ||
            (char_before_cursor == '{' && char_after_cursor == '}') ||
            (char_before_cursor == '\'' && char_after_cursor == '\'') ||
            (char_before_cursor == '"' && char_after_cursor == '"')) {
            memmove(&e->data.items[cursor_pos - 1], &e->data.items[cursor_pos + 1], e->data.count - cursor_pos);
            e->cursor -= 1;
            e->data.count -= 2;
        } else if (editor_is_line_empty(e, row)) {
          if (row > 0) {
            // If it's not the first line, delete the newline character from the previous line
            size_t newline_pos = e->lines.items[row - 1].end; // Position of newline character
            memmove(&e->data.items[newline_pos], &e->data.items[newline_pos + 1], e->data.count - newline_pos - 1);
            e->cursor = newline_pos; // Move cursor to the end of the previous line
            e->data.count -= 1;
          } else if (e->lines.count > 1) {
            // If it's the first line but there are more lines, delete the newline character at the end of this line
            size_t newline_pos = e->lines.items[row].end; // Position of newline character
            memmove(&e->data.items[newline_pos], &e->data.items[newline_pos + 1], e->data.count - newline_pos - 1);
            e->data.count -= 1;
            // Cursor stays at the beginning of the next line (which is now the first line)
          }
        } else if (editor_is_line_whitespaced(e, row)) {
            /* // If the line is only whitespaces */
            /* size_t line_begin = e->lines.items[row].begin; */
            /* size_t delete_length = (cursor_pos - line_begin >= indentation) ? indentation : cursor_pos - line_begin; */

            /* memmove(&e->data.items[cursor_pos - delete_length], &e->data.items[cursor_pos], e->data.count - cursor_pos); */
            /* e->cursor -= delete_length; */
            /* e->data.count -= delete_length; */

            // If the line is only whitespaces
            size_t line_begin = e->lines.items[row].begin;
            size_t line_end = e->lines.items[row].end;
            size_t whitespace_length = cursor_pos - line_begin;
            
            if (whitespace_length == indentation) {
                // If the number of whitespaces matches indentation exactly, remove the entire line
                if (row < e->lines.count - 1) {
                    memmove(&e->data.items[line_begin], &e->data.items[line_end + 1], e->data.count - line_end - 1);
                    e->data.count -= (line_end - line_begin + 1);
                    e->cursor = line_begin; // Update cursor position to the beginning of the next line
                } else if (row > 0 && e->data.items[line_begin - 1] == '\n') {
                    // If it's the last line, remove the preceding newline character
                    e->data.count -= 1;
                    memmove(&e->data.items[line_begin - 1], &e->data.items[line_end], e->data.count - line_end);
                    e->cursor = (line_begin > 1) ? line_begin - 1 : 0; // Move cursor to the end of the previous line, plus one character
                }
                // Update the cursor position if it's not the first line
                if (row > 0) {
                    e->cursor = e->lines.items[row - 1].end; // Move cursor to one character right of the end of the previous line
                    if (e->cursor > e->data.count) e->cursor = e->data.count; // Bound check
                }
            } else {
                // Original behavior for deleting whitespaces
                size_t delete_length = (whitespace_length >= indentation) ? indentation : whitespace_length;
                memmove(&e->data.items[cursor_pos - delete_length], &e->data.items[cursor_pos], e->data.count - cursor_pos);
                e->cursor -= delete_length;
                e->data.count -= delete_length;
            }
        } else {
            // Delete only the character before the cursor
            memmove(&e->data.items[cursor_pos - 1], &e->data.items[cursor_pos], e->data.count - cursor_pos);
            e->cursor -= 1;
            e->data.count -= 1;
        }
        editor_retokenize(e);
    }
}


// Unused ?
void editor_delete(Editor *e)
{
    if (e->searching) return;

    if (e->cursor >= e->data.count) return;
    memmove(
        &e->data.items[e->cursor],
        &e->data.items[e->cursor + 1],
        e->data.count - e->cursor - 1
    );
    e->data.count -= 1;
    editor_retokenize(e);
}

void editor_delete_selection(Editor *e)
{
    assert(e->selection);

    size_t begin = e->select_begin;
    size_t end = e->cursor;
    if (begin > end) {
        SWAP(size_t, begin, end);
    }

    if (end >= e->data.count) {
        end = e->data.count - 1;
    }
    if (begin == e->data.count) return;

    size_t nchars = end - begin + 1; // Correct calculation to include the end character

    memmove(
        &e->data.items[begin],
        &e->data.items[end + 1],
        e->data.count - end - 1
    );

    e->data.count -= nchars;
    e->cursor = begin; // Set cursor to the beginning of the deleted range

    editor_retokenize(e);
}


// TODO: make sure that you always have new line at the end of the file while saving
// https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html#tag_03_206

Errno editor_save_as(Editor *e, const char *file_path)
{
    printf("Saving as %s...\n", file_path);
    Errno err = write_entire_file(file_path, e->data.items, e->data.count);
    if (err != 0) return err;
    e->file_path.count = 0;
    sb_append_cstr(&e->file_path, file_path);
    sb_append_null(&e->file_path);
    return 0;
}

Errno editor_save(const Editor *e)
{
    assert(e->file_path.count > 0);
    printf("Saving as %s...\n", e->file_path.items);
    return write_entire_file(e->file_path.items, e->data.items, e->data.count);
}

/* Errno editor_load_from_file(Editor *e, const char *file_path) */
/* { */
/*     printf("Loading %s\n", file_path); */

/*     e->data.count = 0; */
/*     Errno err = read_entire_file(file_path, &e->data); */
/*     if (err != 0) return err; */

/*     e->cursor = 0; */

/*     editor_retokenize(e); */

/*     e->file_path.count = 0; */
/*     sb_append_cstr(&e->file_path, file_path); */
/*     sb_append_null(&e->file_path); */

/*     // Add file path to buffer history */
/*     if (e->buffer_history_count < MAX_BUFFER_HISTORY) { */
/*         e->buffer_history[e->buffer_history_count++] = strdup(file_path); */
/*     } */

/*     return 0; */
/* } */


size_t get_position_from_line_column(Editor *e, size_t line, size_t column) {
    size_t pos = 0;
    size_t current_line = 0;

    while (pos < e->data.count && current_line < line) {
        if (e->data.items[pos] == '\n') {
            current_line++;
        }
        pos++;
    }

    // Adjust column position
    size_t line_start = pos;
    size_t current_column = 0;
    while (pos < e->data.count && current_column < column) {
        if (e->data.items[pos] == '\n') {
            break; // Prevent going to next line
        }
        current_column++;
        pos++;
    }

    return line_start + current_column;
}

Errno find_file(Editor *e, const char *file_path, size_t line, size_t column) {
    char expanded_file_path[PATH_MAX];
    expand_path(file_path, expanded_file_path, sizeof(expanded_file_path));

    printf("[find_file] Requested File: %s\n", file_path);
    printf("[find_file] Expanded File Path: %s\n", expanded_file_path);
    printf("[find_file] Line: %zu, Column: %zu\n", line, column);

    e->data.count = 0;
    Errno err = read_entire_file(expanded_file_path, &e->data);
    if (err != 0) {
        printf("[find_file] Error reading file: %d\n", err);
        return err;
    }

    size_t line_count = 0;
    for (size_t i = 0; i < e->data.count && line_count <= long_file_lines; ++i) {
        if (e->data.items[i] == '\n') {
            line_count++;
        }
    }

    if (line_count > long_file_lines) {
        if (show_line_numbers_opening_long_files) showLineNumbers = true;
        if (decenter_text_opening_long_files) centeredText = false;
    } else {
        if (hide_line_numbers_opening_small_files) showLineNumbers = false;
        if (center_text_opening_small_files) centeredText = true;
    }

    e->cursor = get_position_from_line_column(e, line, column);
    editor_retokenize(e);

    e->file_path.count = 0;
    sb_append_cstr(&e->file_path, expanded_file_path);
    sb_append_null(&e->file_path);

    if (e->buffer_history_count < MAX_BUFFER_HISTORY) {
        e->buffer_history[e->buffer_history_count++] = strdup(expanded_file_path);
    }

    printf("[find_file] File loaded and cursor set.\n");
    printf("[find_file] Read only: %d\n", readonly);
    return 0;
}

size_t editor_cursor_row(const Editor *e)
{
    assert(e->lines.count > 0);
    for (size_t row = 0; row < e->lines.count; ++row) {
        Line line = e->lines.items[row];
        if (line.begin <= e->cursor && e->cursor <= line.end) {
            return row;
        }
    }
    return e->lines.count - 1;
}


void get_cursor_position(const Editor *e) {
  /* assert(e != NULL && line != NULL && character != NULL); */

  // Get the line number
  size_t line = editor_cursor_row(e);

  // Find the start of the current line
  size_t line_start = 0;
  if (line > 0 && line < e->lines.count) {
    line_start = e->lines.items[line].begin;
  }

  // Calculate the column number (character position)
  int character = e->cursor - line_start;
}

void editor_move_line_up(Editor *e)
{
    editor_stop_search(e);

    size_t cursor_row = editor_cursor_row(e);
    size_t cursor_col = e->cursor - e->lines.items[cursor_row].begin;
    if (cursor_row > 0) {
        Line next_line = e->lines.items[cursor_row - 1];
        size_t next_line_size = next_line.end - next_line.begin;
        if (cursor_col > next_line_size) cursor_col = next_line_size;
        e->cursor = next_line.begin + cursor_col;
    }
}

void editor_move_line_down(Editor *e)
{
    editor_stop_search(e);

    size_t cursor_row = editor_cursor_row(e);
    size_t cursor_col = e->cursor - e->lines.items[cursor_row].begin;
    if (cursor_row < e->lines.count - 1) {
        Line next_line = e->lines.items[cursor_row + 1];
        size_t next_line_size = next_line.end - next_line.begin;
        if (cursor_col > next_line_size) cursor_col = next_line_size;
        e->cursor = next_line.begin + cursor_col;
    }
}

void editor_move_char_left(Editor *e)
{
    editor_stop_search(e);
    if (e->cursor > 0) e->cursor -= 1;
}

void editor_move_char_right(Editor *e)
{
    editor_stop_search(e);
    if (e->cursor < e->data.count) e->cursor += 1;
}

void editor_move_word_left(Editor *e)
{
    editor_stop_search(e);
    while (e->cursor > 0 && !isalnum(e->data.items[e->cursor - 1])) {
        e->cursor -= 1;
    }
    while (e->cursor > 0 && isalnum(e->data.items[e->cursor - 1])) {
        e->cursor -= 1;
    }
}

void editor_move_word_right(Editor *e)
{
    editor_stop_search(e);
    while (e->cursor < e->data.count && !isalnum(e->data.items[e->cursor])) {
        e->cursor += 1;
    }
    while (e->cursor < e->data.count && isalnum(e->data.items[e->cursor])) {
        e->cursor += 1;
    }
}

void editor_insert_char(Editor *e, char x)
{
    editor_insert_buf(e, &x, 1);
}

void editor_insert_char_at(Editor *e, char c, size_t pos) {
    editor_insert_buf_at(e, &c, 1, pos);
}



void editor_insert_buf(Editor *e, char *buf, size_t buf_len)
{
    if (e->searching) {
        sb_append_buf(&e->search, buf, buf_len);
        bool matched = false;
        for (size_t pos = e->cursor; pos < e->data.count; ++pos) {
            if (editor_search_matches_at(e, pos)) {
                e->cursor = pos;
                matched = true;
                break;
            }
        }
        if (!matched) e->search.count -= buf_len;
    } else if (e->minibuffer_active) {
        sb_append_buf(&e->minibuffer_text, buf, buf_len);
        /* printf("Minibuffer: "SB_Fmt"\n", SB_Arg(e->minibuffer_text)); */
    } else {
        if (e->cursor > e->data.count) {
            e->cursor = e->data.count;
        }

        for (size_t i = 0; i < buf_len; ++i) {
            da_append(&e->data, '\0');
        }
        memmove(
            &e->data.items[e->cursor + buf_len],
            &e->data.items[e->cursor],
            e->data.count - e->cursor - buf_len
        );
        memcpy(&e->data.items[e->cursor], buf, buf_len);
        e->cursor += buf_len;
        editor_retokenize(e);
        /* printf("%.*s", (int)buf_len, buf); */
    }
}


void editor_insert_buf_at(Editor *e, char *buf, size_t buf_len, size_t pos) {
    // Ensure the position is within bounds
    if (pos > e->data.count) {
        pos = e->data.count;
    }

    // Expand the buffer to accommodate the new text
    for (size_t i = 0; i < buf_len; ++i) {
        da_append(&e->data, '\0');
    }

    // Shift existing text to make room for the new text
    memmove(&e->data.items[pos + buf_len], &e->data.items[pos], e->data.count - pos);

    // Copy the new text into the buffer at the specified position
    memcpy(&e->data.items[pos], buf, buf_len);

    // Update the cursor position and retokenize
    e->cursor = pos + buf_len;
    editor_retokenize(e);
}

void editor_retokenize(Editor *e)
{
    // Lines
    {
        e->lines.count = 0;

        Line line;
        line.begin = 0;

        for (size_t i = 0; i < e->data.count; ++i) {
            if (e->data.items[i] == '\n') {
                line.end = i;
                da_append(&e->lines, line);
                line.begin = i + 1;
            }
        }

        line.end = e->data.count;
        da_append(&e->lines, line);
    }

    // Syntax Highlighting
    {
        e->tokens.count = 0;
        Lexer l = lexer_new(e->atlas, e->data.items, e->data.count);
        /* Lexer l = lexer_new(e->atlas, e->data.items, e->data.count, e->file_path); */
        Token t = lexer_next(&l);
        while (t.kind != TOKEN_END) {
            da_append(&e->tokens, t);
            t = lexer_next(&l);
        }
    }
}

bool editor_line_starts_with(Editor *e, size_t row, size_t col, const char *prefix)
{
    size_t prefix_len = strlen(prefix);
    if (prefix_len == 0) {
        return true;
    }
    Line line = e->lines.items[row];
    if (col + prefix_len - 1 >= line.end) {
        return false;
    }
    for (size_t i = 0; i < prefix_len; ++i) {
        if (prefix[i] != e->data.items[line.begin + col + i]) {
            return false;
        }
    }
    return true;
}

const char *editor_line_starts_with_one_of(Editor *e, size_t row, size_t col, const char **prefixes, size_t prefixes_count)
{
    for (size_t i = 0; i < prefixes_count; ++i) {
        if (editor_line_starts_with(e, row, col, prefixes[i])) {
            return prefixes[i];
        }
    }
    return NULL;
}

void editor_clipboard_copy(Editor *e)
{
    if (e->searching) return;
    if (e->selection) {
        size_t begin = e->select_begin;
        size_t end = e->cursor;
        if (begin > end) SWAP(size_t, begin, end);

        e->clipboard.count = 0;
        sb_append_buf(&e->clipboard, &e->data.items[begin], end - begin + 1);
        sb_append_null(&e->clipboard);

        if (SDL_SetClipboardText(e->clipboard.items) < 0) {
            fprintf(stderr, "ERROR: SDL ERROR: %s\n", SDL_GetError());
        }
    }
    copiedLine = false;
}

void editor_clipboard_paste(Editor *e)
{
    char *text = SDL_GetClipboardText();
    size_t text_len = strlen(text);
    if (text_len > 0) {
        editor_insert_buf(e, text, text_len);
    } else {
        fprintf(stderr, "ERROR: SDL ERROR: %s\n", SDL_GetError());
    }
    SDL_free(text);
}

void editor_update_selection(Editor *e, bool shift) {
    if (e->searching) return;
    
     if (shift) {
        if (!e->selection) {
            e->selection = true;
            e->select_begin = e->cursor;
        }
     } else if (current_mode == VISUAL_LINE) {
         
     } else if (current_mode == VISUAL) {

     } else {
         e->selection = false;
     }
}


// search
void editor_start_search(Editor *e)
{
    if (e->searching) {
        for (size_t pos = e->cursor + 1; pos < e->data.count; ++pos) {
            if (editor_search_matches_at(e, pos)) {
                e->cursor = pos;
                break;
            }
        }
    } else {
        e->searching = true;
        if (e->selection) {
            e->selection = false;
            // TODO: put the selection into the search automatically
        } else {
            e->search.count = 0;
        }
    }
}

void editor_stop_search(Editor *e)
{
    e->searching = false;
}

void editor_stop_search_and_mark(Editor *e) {
    e->searching = false;

    e->has_mark = true;  // Mark the search result.
    e->mark_start = e->cursor;
    e->mark_end = e->cursor + e->search.count;
}

void editor_clear_mark(Editor *editor) {
    editor->has_mark = false;
    editor->mark_start = 0;  // or some other appropriate default value
    editor->mark_end = 0;    // or some other appropriate default value
}

bool editor_search_matches_at(Editor *e, size_t pos)
{
    if (e->data.count - pos < e->search.count) return false;
    for (size_t i = 0; i < e->search.count; ++i) {
        if (e->search.items[i] != e->data.items[pos + i]) {
            return false;
        }
    }
    return true;
}

void editor_move_to_begin(Editor *e)
{
    editor_stop_search(e);
    e->cursor = 0;
}

void editor_move_to_end(Editor *e)
{
    editor_stop_search(e);
    e->cursor = e->data.count;
}

void editor_move_to_line_begin(Editor *e)
{
    editor_stop_search(e);
    size_t row = editor_cursor_row(e);
    e->cursor = e->lines.items[row].begin;
}

void editor_move_to_line_end(Editor *e)
{
    editor_stop_search(e);
    size_t row = editor_cursor_row(e);
    e->cursor = e->lines.items[row].end;
}

void editor_move_paragraph_up(Editor *e)
{
    editor_stop_search(e);
    size_t row = editor_cursor_row(e);
    while (row > 0 && e->lines.items[row].end - e->lines.items[row].begin <= 1) {
        row -= 1;
    }
    while (row > 0 && e->lines.items[row].end - e->lines.items[row].begin > 1) {
        row -= 1;
    }
    e->cursor = e->lines.items[row].begin;
}

void editor_move_paragraph_down(Editor *e)
{
    editor_stop_search(e);
    size_t row = editor_cursor_row(e);
    while (row + 1 < e->lines.count && e->lines.items[row].end - e->lines.items[row].begin <= 1) {
        row += 1;
    }
    while (row + 1 < e->lines.count && e->lines.items[row].end - e->lines.items[row].begin > 1) {
        row += 1;
    }
    e->cursor = e->lines.items[row].begin;
}

ssize_t find_matching_parenthesis(Editor *editor, size_t cursor_pos) {
    // Ensure the cursor position is within the valid range
    if (cursor_pos >= editor->data.count) return -1;
    if (matchParenthesis){
        char current_char = editor->data.items[cursor_pos];
        char matching_char;
        int direction;
        
        // Check if the character at cursor is a parenthesis
        switch (current_char) {
        case '(': matching_char = ')'; direction = 1; break;
        case ')': matching_char = '('; direction = -1; break;
        case '[': matching_char = ']'; direction = 1; break;
        case ']': matching_char = '['; direction = -1; break;
        case '{': matching_char = '}'; direction = 1; break;
        case '}': matching_char = '{'; direction = -1; break;
        default: return -1; // Not on a parenthesis character
        }
        
        int balance = 1;
        size_t pos = cursor_pos;
        
        while ((direction > 0 && pos < editor->data.count - 1) || (direction < 0 && pos > 0)) {
            pos += direction;
            
            if (editor->data.items[pos] == current_char) {
                balance++;
            } else if (editor->data.items[pos] == matching_char) {
                balance--;
                if (balance == 0) {
                    return pos; // Found the matching parenthesis
                }
            }
        }
        return -1; // No matching parenthesis found
    }
    // if matchParenthesis is false or any other condition not covered above
    return -1;
}


void editor_enter(Editor *e) {
    if (e->searching) {
        editor_stop_search_and_mark(e);
        current_mode = NORMAL;
        return;
    } else if ((M_x_active || evil_command_active) && e->minibuffer_active) {
        sb_append_null(&e->minibuffer_text); // null termination
        execute_command(e->commands, e, e->minibuffer_text.items);
        e->minibuffer_text.count = 0;
        e->minibuffer_active = false;
        M_x_active = false;
        current_mode = NORMAL;
    } else {
        size_t row = editor_cursor_row(e);
        size_t line_end = e->lines.items[row].end;
        
        editor_insert_char(e, '\n');
        size_t line_begin = e->lines.items[row].begin;
        bool inside_braces = false;
        
        // Check if the line contains an opening brace '{'
        for (size_t i = line_begin; i < line_end; ++i) {
            char c = e->data.items[i];
            if (c == '{') {
                inside_braces = true;
                break;
            }
        }
        
        // Insert the same whitespace character
        for (size_t i = line_begin; i < line_end; ++i) {
            char c = e->data.items[i];
            if (c == ' ' || c == '\t') {
                editor_insert_char(e, c);
            } else {
                break;
            }
        }
        
        // If inside braces, perform additional steps
        if (inside_braces) {
            editor_move_line_up(e);
            editor_move_to_line_end(e);
            editor_insert_char(e, '\n');
            
            // Add indentation
            for (size_t i = 0; i < indentation; ++i) {
                editor_insert_char(e, ' ');
            }
        }

        indent(e);

        e->last_stroke = SDL_GetTicks();
    }
}


// Anchor Implementation: Initially, the anchor used a single index from the
// start of the buffer, requiring updates on text changes. To simplify, we now
// track two indices (start and end of buffer). The anchor position self-adjusts
// based on cursor's relative position, ensuring correct placement without
// modifying all text-manipulating functions still a dumb implementation.

void editor_set_anchor(Editor *editor) {
    if (editor->cursor < editor->data.count) {
        editor->has_anchor = true;
        editor->anchor_pos_from_start = editor->cursor;
        editor->anchor_pos_from_end = editor->data.count - editor->cursor;
    }
}

void editor_goto_anchor_and_clear(Editor *editor) {
    if (editor->has_anchor) {
        if (editor->cursor > editor->anchor_pos_from_start) {
            editor->cursor = editor->anchor_pos_from_start;
        } else {
            editor->cursor = editor->data.count - editor->anchor_pos_from_end;
        }
        editor->has_anchor = false;
    }
}

void editor_update_anchor(Editor *editor) {
    if (!editor->has_anchor) return;

    if (editor->cursor > editor->anchor_pos_from_start) {
        // Cursor is after the anchor, use position from the start
        editor->anchor_pos = editor->anchor_pos_from_start;
    } else {
        // Cursor is before the anchor, use position from the end
        editor->anchor_pos = editor->data.count - editor->anchor_pos_from_end;
    }
}



// TODO refactor and implement drag region (selection)
void editor_drag_line_down(Editor *editor) {
    size_t row = editor_cursor_row(editor);
    if (row >= editor->lines.count - 1) return; // Can't move the last line down

    Line current_line = editor->lines.items[row];
    Line next_line = editor->lines.items[row + 1];

    // Calculate lengths including the newline character
    size_t current_line_length = current_line.end - current_line.begin + 1;
    size_t next_line_length = next_line.end - next_line.begin + 1;

    // Allocate temporary buffer to hold the lines
    char *temp = malloc(current_line_length + next_line_length);
    if (!temp) {
        // Handle memory allocation error
        fprintf(stderr, "ERROR: Unable to allocate memory for line swapping.\n");
        return;
    }

    // Copy current and next lines into temp
    memcpy(temp, &editor->data.items[current_line.begin], current_line_length);
    memcpy(temp + current_line_length, &editor->data.items[next_line.begin], next_line_length);

    // Swap lines in editor's data
    memcpy(&editor->data.items[current_line.begin], temp + current_line_length, next_line_length);
    memcpy(&editor->data.items[current_line.begin + next_line_length], temp, current_line_length);

    // Free the temporary buffer
    free(temp);

    // Update cursor position
    if (editor->cursor >= current_line.begin && editor->cursor < current_line.end) {
        // The cursor is on the current line, move it down with the line
        editor->cursor += next_line_length;
    } else if (editor->cursor >= next_line.begin && editor->cursor <= next_line.end) {
        // The cursor is on the next line, move it up to the start of the current line
        editor->cursor = current_line.begin + (editor->cursor - next_line.begin);
    }

    // Update line positions in the Lines struct
    editor->lines.items[row].begin = current_line.begin;
    editor->lines.items[row].end = current_line.begin + next_line_length - 1;
    editor->lines.items[row + 1].begin = current_line.begin + next_line_length;
    editor->lines.items[row + 1].end = editor->lines.items[row + 1].begin + current_line_length - 1;

    // Retokenize
    editor_retokenize(editor);
}

void editor_drag_line_up(Editor *editor) {
    size_t row = editor_cursor_row(editor);
    if (row == 0) return; // Can't move the first line up

    Line current_line = editor->lines.items[row];
    Line previous_line = editor->lines.items[row - 1];

    // Calculate lengths including the newline character
    size_t current_line_length = current_line.end - current_line.begin + 1;
    size_t previous_line_length = previous_line.end - previous_line.begin + 1;

    // Allocate temporary buffer to hold the lines
    char *temp = malloc(current_line_length + previous_line_length);
    if (!temp) {
        // Handle memory allocation error
        fprintf(stderr, "ERROR: Unable to allocate memory for line swapping.\n");
        return;
    }

    // Copy current and previous lines into temp
    memcpy(temp, &editor->data.items[previous_line.begin], previous_line_length);
    memcpy(temp + previous_line_length, &editor->data.items[current_line.begin], current_line_length);

    // Swap lines in editor's data
    memcpy(&editor->data.items[previous_line.begin], temp + previous_line_length, current_line_length);
    memcpy(&editor->data.items[previous_line.begin + current_line_length], temp, previous_line_length);

    // Free the temporary buffer
    free(temp);

    // Update cursor position
    editor->cursor = previous_line.begin + (editor->cursor - current_line.begin);

    // Update line positions in the Lines struct
    editor->lines.items[row - 1].begin = previous_line.begin;
    editor->lines.items[row - 1].end = previous_line.begin + current_line_length - 1;
    editor->lines.items[row].begin = previous_line.begin + current_line_length;
    editor->lines.items[row].end = editor->lines.items[row].begin + previous_line_length - 1;

    // Retokenize
    editor_retokenize(editor);
}




void add_one_indentation_here(Editor *editor) {
    for (size_t i = 0; i < indentation; ++i) {
        editor_insert_char(editor, ' ');
    }
}

void remove_one_indentation_here(Editor *editor) {
    for (size_t i = 0; i < indentation; ++i) {
        editor_delete(editor);
    }
}

void add_one_indentation(Editor *editor) {
    size_t cursor_row = editor_cursor_row(editor);
    Line currentLineData = editor->lines.items[cursor_row];
    size_t originalCursorPosition = editor->cursor;

    // Calculate current indentation of the line
    size_t currentIndentation = 0;
    for (size_t i = currentLineData.begin; i < currentLineData.end && isspace(editor->data.items[i]); ++i) {
        currentIndentation++;
    }

    // Move cursor to the beginning of the current line
    editor->cursor = currentLineData.begin;

    // Add one level of indentation at the beginning of the line
    for (size_t i = 0; i < indentation; ++i) {
        editor_insert_char(editor, ' ');
    }

    // Adjust cursor position
    if (originalCursorPosition <= currentLineData.begin + currentIndentation) {
        // If the cursor was at or before the first non-whitespace character, move it right after the added indentation
        editor->cursor = currentLineData.begin + currentIndentation + indentation;
    } else {
        // If the cursor was on a non-whitespace character, maintain relative position
        editor->cursor = originalCursorPosition + indentation;
    }
}

void remove_one_indentation(Editor *editor) {
    size_t cursor_row = editor_cursor_row(editor);
    Line currentLineData = editor->lines.items[cursor_row];

    // Save the current cursor position
    size_t originalCursorPosition = editor->cursor;

    // Calculate current indentation of the line
    size_t currentIndentation = 0;
    size_t firstNonWhitespace = currentLineData.begin;
    while (firstNonWhitespace < currentLineData.end && isspace(editor->data.items[firstNonWhitespace])) {
        currentIndentation++;
        firstNonWhitespace++;
    }

    // Check if there's at least one indentation level to remove
    if (currentIndentation >= indentation) {
        // Move cursor to the beginning of the current line
        editor->cursor = currentLineData.begin;

        // Remove one level of indentation from the beginning of the line
        for (size_t i = 0; i < indentation; ++i) {
            editor_delete(editor); // Assuming delete removes one character.
        }

        // Adjust cursor position
        if (originalCursorPosition <= currentLineData.begin + currentIndentation) {
            // If the cursor was within the leading whitespace, move it to the first non-whitespace character
            editor->cursor = firstNonWhitespace - indentation;
        } else {
            // If the cursor was on a non-whitespace character, maintain relative position
            editor->cursor = originalCursorPosition - indentation;
        }
    }
}



// TODO slow calculation on whitespaces
void indent(Editor *editor) {
    size_t cursor_row = editor_cursor_row(editor);
    int braceLevel = 0;

    // Calculate brace level up to the current line
    for (size_t i = 0; i < cursor_row; ++i) {
        Line line = editor->lines.items[i];
        for (size_t j = line.begin; j < line.end; ++j) {
            char c = editor->data.items[j];
            if (c == '{') {
                braceLevel++;
            } else if (c == '}') {
                braceLevel = (braceLevel > 0) ? braceLevel - 1 : 0;
            }
        }
    }

    Line currentLineData = editor->lines.items[cursor_row];
    bool decreaseIndentation = false;
    size_t firstNonWhitespace = currentLineData.begin;
    bool isLineEmpty = true;
    for (size_t j = currentLineData.begin; j < currentLineData.end; ++j) {
        char c = editor->data.items[j];
        if (!isspace(c)) {
            firstNonWhitespace = j;
            isLineEmpty = false;
            if (c == '}') {
                decreaseIndentation = true;
            }
            break;
        }
    }

    // Adjust brace level for current line if it starts with a closing brace
    if (decreaseIndentation) {
        braceLevel = (braceLevel > 0) ? braceLevel - 1 : 0;
    }

    // Calculate required and current indentation
    size_t requiredIndentation = braceLevel * indentation;
    size_t currentIndentation = 0;
    for (size_t i = currentLineData.begin; i < currentLineData.end && (editor->data.items[i] == ' ' || editor->data.items[i] == '\t'); ++i) {
        currentIndentation++;
    }

    // Save the current cursor position
    size_t originalCursorPosition = editor->cursor;

    // Adjust indentation
    editor->cursor = currentLineData.begin;
    while (currentIndentation < requiredIndentation) {
        editor_insert_char(editor, ' ');
        currentIndentation++;
    }

    while (currentIndentation > requiredIndentation && currentIndentation > 0) {
        editor_delete(editor); // or evil_delete_char(editor);
        currentIndentation--;
    }

    // Adjust cursor position based on initial condition
    if (isLineEmpty || originalCursorPosition <= firstNonWhitespace) {
        // If the line is empty or the cursor was on or before the first non-whitespace character
        editor->cursor = currentLineData.begin + requiredIndentation;
    } else {
        // If the cursor was on a non-whitespace character, maintain relative position
        size_t characterOffset = originalCursorPosition - firstNonWhitespace;
        editor->cursor = currentLineData.begin + requiredIndentation + characterOffset;
    }
}



bool extractLocalIncludePath(Editor *editor, char *includePath) {
    char line[512]; // Adjust size as needed
    if (!extractLine(editor, editor->cursor, line, sizeof(line))) {
        return false;
    }

    if (strncmp(line, "#include \"", 10) == 0) {
        char *start = strchr(line, '\"') + 1;
        char *end = strrchr(line, '\"');
        if (start && end && start < end) {
            size_t length = end - start;
            strncpy(includePath, start, length);
            includePath[length] = '\0';
            return true;
        }
    }

    return false;
}

void getDirectoryFromFilePath(const char *filePath, char *directory) {
    strcpy(directory, filePath);
    char *lastSlash = strrchr(directory, '/');
    if (lastSlash != NULL) {
        *lastSlash = '\0'; // Null-terminate at the last slash
    } else {
        // Handle the case where there is no slash in the path
        strcpy(directory, ".");
    }
}

Errno openLocalIncludeFile(Editor *editor, const char *includePath) {
    char fullPath[512]; // Buffer for the full path
    char directory[256]; // Buffer for the directory

    // Get the directory of the current file
    getDirectoryFromFilePath(editor->file_path.items, directory);

    // Construct the full path
    snprintf(fullPath, sizeof(fullPath), "%s/%s", directory, includePath);

    // Load the file using the full path
    Errno load_err = find_file(editor, fullPath, 10, 10);
    if (load_err != 0) {
        fprintf(stderr, "Error loading file %s: %s\n", fullPath, strerror(load_err));
        return load_err;
    }

    printf("Opened file: %s\n", fullPath);
    return 0;
}

bool extractGlobalIncludePath(Editor *editor, char *includePath) {
    char line[512];
    if (!extractLine(editor, editor->cursor, line, sizeof(line))) {
        return false;
    }

    if (strncmp(line, "#include <", 10) == 0) {
        char *start = strchr(line, '<') + 1;
        char *end = strrchr(line, '>');
        if (start && end && start < end) {
            size_t length = end - start;
            strncpy(includePath, start, length);
            includePath[length] = '\0';
            return true;
        }
    }

    return false;
}

#include "unistd.h" // for F_OK
Errno openGlobalIncludeFile(Editor *editor, const char *includePath) {
    char fullPath[512]; // Buffer for the full path

    // List of standard directories (expandable)
    const char *standardDirs[] = {"/usr/include", NULL}; // NULL terminated array

    for (int i = 0; standardDirs[i] != NULL; i++) {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", standardDirs[i], includePath);

        // Check if the file exists and is accessible
        if (access(fullPath, F_OK) != -1) {
            // Try to load the file using the constructed full path
            Errno load_err = find_file(editor, fullPath, 0, 0);
            if (load_err == 0) {
                printf("Opened file: %s\n", fullPath);
                return 0; // File opened successfully
            }
        }
    }

    // Print the error message only if the file is not found in /usr/include
    fprintf(stderr, "Error: File %s not found in /usr/include\n", includePath);
    return -1; // File not found in /usr/include
}

void editor_open_include(Editor *editor) {
    char includePath[256];

    if (extractLocalIncludePath(editor, includePath)) {
        openLocalIncludeFile(editor, includePath);
    } else if (extractGlobalIncludePath(editor, includePath)) {
        openGlobalIncludeFile(editor, includePath);
    }
}










// CLANG FORMAT TODO
#include <stdlib.h>

int is_clang_format_installed() {
    if (system("clang-format --version") != 0) {
        return 0;
    }
    return 1;
}

void clang_format(const char *filename, const char *arguments) {
    if (!is_clang_format_installed()) {
        printf("bruh clang-format is not installed.\n");
        return;
    }

    char command[1024];
    snprintf(command, sizeof(command), "clang-format %s %s", arguments, filename);

    // Execute the command
    int result = system(command);
    if (result != 0) {
        printf("Error executing clang-format.\n");
    }
}


// TODO select more after end brace
void select_region_from_inside_braces(Editor *editor) {
    if (editor->cursor >= editor->data.count) return;

    size_t row = editor_cursor_row(editor);
    size_t start = row;
    size_t end = row;

    // Find the start of the function
    while (start > 0) {
        start--;
        size_t line_end = editor->lines.items[start].end;

        // Simple heuristic: a line ending with '{' might be the start of a function
        if (editor->data.items[line_end - 1] == '{') {
            break;
        }
    }

    // Find the end of the function
    int brace_count = 1; // Start after finding the opening brace
    while (end < editor->lines.count - 1) {
        end++;
        size_t line_begin = editor->lines.items[end].begin;
        size_t line_end = editor->lines.items[end].end;

        for (size_t i = line_begin; i < line_end; i++) {
            if (editor->data.items[i] == '{') {
                brace_count++;
            } else if (editor->data.items[i] == '}') {
                brace_count--;
                if (brace_count == 0) {
                    // Found the matching closing brace
                    goto found_end;
                }
            }
        }
    }
found_end:

    // Update the selection
    editor->selection = true;
    editor->select_begin = editor->lines.items[start].begin;
    editor->cursor = editor->lines.items[end].end;
}


// TODO should not run from anywhere just curly braces
// TODO dont move the cursor on open brace like it does for closing brace
void select_region_from_brace(Editor *editor) {
    if (editor->cursor >= editor->data.count) return;

    char current_char = editor->data.items[editor->cursor];

    if (strchr("})", current_char)) {
        // Called from a closing brace
        editor->select_begin = editor->cursor;
        evil_jump_item(editor);
        size_t row = editor_cursor_row(editor);
        editor->cursor = editor->lines.items[row].begin; // Extend to the beginning of the line
    } else if (strchr("({", current_char)) {
        // Called from an opening brace
        size_t row = editor_cursor_row(editor);
        editor->select_begin = editor->lines.items[row].begin; // Start from the beginning of the line
        evil_jump_item(editor);
        row = editor_cursor_row(editor);
        editor->cursor = editor->lines.items[row].end; // Extend to the end of the line with the closing brace
    }

    // Update the selection
    editor->selection = true;
    if (editor->select_begin > editor->cursor) {
        // Ensure select_begin is always before the cursor
        size_t temp = editor->select_begin;
        editor->select_begin = editor->cursor;
        editor->cursor = temp;
    }
}


// TODO select_function


bool toggle_bool(Editor *editor) {
    char word[256];
    if (!extract_word_under_cursor(editor, word)) {
        return false;
    }

    const char *replacement = NULL;
    int difference = 0;
    if (strcmp(word, "true") == 0) {
        replacement = "false";
        difference = 1; // "false" is 1 character longer than "true"
    } else if (strcmp(word, "false") == 0) {
        replacement = "true";
        difference = -1; // "true" is 1 character shorter than "false"
    } else {
        return false;
    }

    // Find the start position of the word
    size_t word_start = editor->cursor;
    while (word_start > 0 && isalnum(editor->data.items[word_start - 1])) {
        word_start--;
    }

    // Shift the buffer contents if necessary
    if (difference != 0) {
        memmove(&editor->data.items[word_start + strlen(replacement)],
                &editor->data.items[word_start + strlen(word)],
                editor->data.count - word_start - strlen(word));
        editor->data.count += difference;
    }

    // Replace the word directly in the buffer
    memcpy(&editor->data.items[word_start], replacement, strlen(replacement));

    editor_retokenize(editor);
    return true;  // Successfully toggled
}

void editor_quit() {
    quit = true;
}

void editor_save_and_quit(Editor *e) {
    editor_save(e);
    quit = true;
}


#define MAX_MATCHES 1024
#define MAX_WORD_LENGTH 256

// TODO cycle

void evil_complete_next(Editor *e) {
    static char last_word[MAX_WORD_LENGTH] = {0};
    static size_t last_match_index = 0;
    char current_word[MAX_WORD_LENGTH];

    if (!extract_word_left_of_cursor(e, current_word, sizeof(current_word))) {
        printf("No word to complete.\n");
        return;
    }

    if (strcmp(last_word, current_word) != 0) {
        strcpy(last_word, current_word);
        last_match_index = 0;
    }

    char *matches[MAX_MATCHES];
    size_t matches_count = 0;
    find_matches_in_editor_data(e, current_word, matches, &matches_count);

    if (matches_count == 0) {
        printf("Pattern not found.\n");
        return;
    }

    const char *next_match = matches[last_match_index % matches_count];
    size_t next_match_length = strlen(next_match);
    size_t current_word_length = strlen(current_word);

    // Adjust the buffer size and content
    if (next_match_length != current_word_length) {
        memmove(&e->data.items[e->cursor + next_match_length],
                &e->data.items[e->cursor + current_word_length],
                e->data.count - e->cursor);
        e->data.count += next_match_length - current_word_length;
    }

    // Replace the current word with the match
    memcpy(&e->data.items[e->cursor], next_match, next_match_length);

    // Update the cursor position to the end of the new word
    e->cursor += next_match_length;

    last_match_index++;

    // Clean up
    for (size_t i = 0; i < matches_count; i++) {
        free(matches[i]);
    }
    editor_retokenize(e);
}

void find_matches_in_editor_data(Editor *e, const char *word, char **matches, size_t *matches_count) {
    size_t word_length = strlen(word);
    *matches_count = 0;
    char *data = e->data.items;
    size_t data_length = e->data.count;

    for (size_t i = 0; i < data_length; i++) {
        if (isalnum(data[i]) && (i == 0 || !isalnum(data[i - 1]))) {
            // Found the start of a word
            if (strncmp(&data[i], word, word_length) == 0) {
                // Found a matching word, now find the end of the word
                size_t word_end = i + 1;
                while (word_end < data_length && isalnum(data[word_end])) {
                    word_end++;
                }

                size_t match_length = word_end - i;
                if (*matches_count < MAX_MATCHES) {
                    matches[*matches_count] = malloc(match_length + 1);
                    strncpy(matches[*matches_count], &data[i], match_length);
                    matches[*matches_count][match_length] = '\0';
                    (*matches_count)++;
                }
            }
        }
    }
}

Errno editor_goto_line(Editor *editor, const char *params[]) {
    if (!params || !params[0]) {
        // Handle error: No line number provided
        return -1;
    }

    size_t line_number = atoi(params[0]);
    if (line_number == 0 || line_number > editor->lines.count) {
        // Line number is out of range
        return -1;
    }

    // Adjust line_number to zero-based index
    line_number -= 1;

    // Set the cursor to the beginning of the specified line
    editor->cursor = editor->lines.items[line_number].begin;

    return 0;
}








// ANIMATIONS

float easeOutCubic(float x) {
    return 1 - pow(1 - x, 3);
}


float targetModelineHeight;
bool isModelineAnimating = false;
void update_modeline_animation() {
    if (!isModelineAnimating) {
        return;
    }

    float animationSpeed = 1.50f;

    if (modelineHeight < targetModelineHeight) {
        modelineHeight += animationSpeed;
        if (modelineHeight > targetModelineHeight) {
            modelineHeight = targetModelineHeight;
        }
    } else if (modelineHeight > targetModelineHeight) {
        modelineHeight -= animationSpeed;
        if (modelineHeight < targetModelineHeight) {
            modelineHeight = targetModelineHeight;
        }
    }

    if (modelineHeight == targetModelineHeight) {
        isModelineAnimating = false;
    }
}


float targetMinibufferHeight;
bool isMinibufferAnimating = false;
float minibufferAnimationProgress = 0.0f; // Normalized progress of the animation
float minibufferAnimationDuration = 1.0f; // Duration of the animation in seconds


void update_minibuffer_animation(float deltaTime) {
    if (!isMinibufferAnimating) {
        return;
    }

    minibufferAnimationProgress += deltaTime / minibufferAnimationDuration;

    if (minibufferAnimationProgress > 1.0f) {
        minibufferAnimationProgress = 1.0f;
        isMinibufferAnimating = false;
    }

    float easedProgress = easeOutCubic(minibufferAnimationProgress);
    minibufferHeight = easedProgress * (targetMinibufferHeight - minibufferHeight) + minibufferHeight;

    if (minibufferHeight == targetMinibufferHeight || minibufferAnimationProgress >= 1.0f) {
        isMinibufferAnimating = false;
    }
}


size_t calculate_max_line_length(const Editor *editor) {
    size_t max_len = 0;
    for (size_t i = 0; i < editor->lines.count; ++i) {
        Line line = editor->lines.items[i];
        size_t line_length = line.end - line.begin;
        if (line_length > max_len) {
            max_len = line_length;
        }
    }
    return max_len;
}

float column_to_x(Free_Glyph_Atlas *atlas, int column) {
    float whitespace_width = measure_whitespace_width(atlas);
    return column * whitespace_width;
}


void editor_color_text_range(Editor *editor, size_t start, size_t end, Vec4f new_color) {
    for (size_t i = 0; i < editor->tokens.count; ++i) {
        Token *token = &editor->tokens.items[i];
        size_t token_start = token->position.x; // Make sure this is the correct way to calculate the start position
        size_t token_end = token_start + token->text_len;

        // Check if the token is within the specified range
        if (token_start < end && token_end > start) {
            token->color = new_color;
        }
    }
}






