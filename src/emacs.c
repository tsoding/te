#include "emacs.h"
#include "utilities.h"


/* void emacs_ungry_delete_backwards(Editor *e) { */
/*     if (e->searching || e->cursor == 0) return; */

/*     size_t original_cursor = e->cursor; */
/*     size_t start_pos = e->cursor; */
/*     size_t last_newline_pos = 0; */
/*     bool found_non_newline_whitespace = false; */
/*     int newline_count = 0; */

/*     // Move left to the start of contiguous whitespace or to the start of the file */
/*     while (start_pos > 0 && isspace(e->data.items[start_pos - 1])) { */
/*         if (e->data.items[start_pos - 1] == '\n') { */
/*             newline_count++; */
/*             last_newline_pos = start_pos - 1; */
/*         } else { */
/*             found_non_newline_whitespace = true; */
/*         } */
/*         start_pos--; */
/*     } */

/*     // If spanning multiple lines, delete but preserve one newline character. */
/*     if (newline_count > 1 || (newline_count == 1 && found_non_newline_whitespace)) { */
/*         start_pos = last_newline_pos + 1; */
/*     } */

/*     size_t length_to_delete = original_cursor - start_pos; */

/*     if (length_to_delete > 0) { */
/*         // Delete */
/*         memmove(&e->data.items[start_pos], &e->data.items[original_cursor], e->data.count - original_cursor); */
/*         e->data.count -= length_to_delete; */
/*         e->cursor = start_pos; */

/*         editor_retokenize(e); */
/*     } */
/* } */

void emacs_ungry_delete_backwards(Editor *e) {
    if (e->searching || e->cursor == 0) return;

    size_t original_cursor = e->cursor;
    size_t start_pos = e->cursor;
    size_t last_newline_pos = 0;
    bool found_non_newline_whitespace = false;
    int newline_count = 0;
    bool should_delete_single_character = true; // Assume we'll delete a single character by default

    // Move left to the start of contiguous whitespace or to the start of the file
    while (start_pos > 0 && isspace(e->data.items[start_pos - 1])) {
        should_delete_single_character = false; // Found whitespace, so we won't be deleting just a single character
        if (e->data.items[start_pos - 1] == '\n') {
            newline_count++;
            last_newline_pos = start_pos - 1;
        } else {
            found_non_newline_whitespace = true;
        }
        start_pos--;
    }

    // If spanning multiple lines, delete but preserve one newline character.
    if ((newline_count > 1 || (newline_count == 1 && found_non_newline_whitespace)) && !should_delete_single_character) {
        start_pos = last_newline_pos + 1;
    } else if (should_delete_single_character) {
        // If no preceding whitespace was found, adjust start_pos to delete just the last character
        start_pos = original_cursor - 1;
    }

    size_t length_to_delete = original_cursor - start_pos;

    if (length_to_delete > 0) {
        // Perform the deletion
        memmove(&e->data.items[start_pos], &e->data.items[original_cursor], e->data.count - original_cursor);
        e->data.count -= length_to_delete;
        e->cursor = start_pos;

        indent(e);
        editor_retokenize(e);
    }
}




// TODO it delete the line if it is on whitespaces even if there is text
void emacs_kill_line(Editor *e) {
    if (e->searching || e->cursor >= e->data.count) return;

    size_t row = editor_cursor_row(e);
    size_t line_begin = e->lines.items[row].begin;
    size_t line_end = e->lines.items[row].end;

    // Check if the line is empty or if the cursor is at the end of the line
    if (line_begin == line_end || e->cursor == line_end) {
        // If the line is empty or the cursor is at the end of the line
        // Remove the newline character if it's not the first line
        if (row < e->lines.count - 1) {
            memmove(&e->data.items[line_begin], &e->data.items[line_end + 1], e->data.count - line_end - 1);
            e->data.count -= (line_end - line_begin + 1);
        } else if (row > 0 && e->data.items[line_begin - 1] == '\n') {
            // If it's the last line, remove the preceding newline character
            e->data.count -= 1;
            memmove(&e->data.items[line_begin - 1], &e->data.items[line_end], e->data.count - line_end);
        }
    } else if (isspace(e->data.items[e->cursor])) {
        // If the cursor is on a whitespace character within the line, delete the entire line
        memmove(&e->data.items[line_begin], &e->data.items[line_end + 1], e->data.count - line_end - 1);
        e->data.count -= (line_end - line_begin + 1);
    } else {
        // If the line is not empty and the cursor is not on a whitespace character, kill the text from the cursor to the end of the line
        size_t length = line_end - e->cursor;

        // Copy the text to be killed to the clipboard
        e->clipboard.count = 0;
        sb_append_buf(&e->clipboard, &e->data.items[e->cursor], length);
        sb_append_null(&e->clipboard);
        if (SDL_SetClipboardText(e->clipboard.items) < 0) {
            fprintf(stderr, "ERROR: SDL ERROR: %s\n", SDL_GetError());
        }

        // Delete the range from the editor
        memmove(&e->data.items[e->cursor], &e->data.items[line_end], e->data.count - line_end);
        e->data.count -= length;
    }

    editor_retokenize(e);
}

// TODO make this work also on search and minibuffer
void emacs_backward_kill_word(Editor *e) {
    editor_stop_search(e); 

    size_t start_pos = e->cursor;

    // Move cursor left to the start of the previous word or to the first capital letter
    if (e->cursor > 0 && isalnum(e->data.items[e->cursor - 1])) {
        // Move left until a non-alphanumeric character or the start of the camelCase word
        while (e->cursor > 0 && isalnum(e->data.items[e->cursor - 1])) {
            e->cursor -= 1;
            if (isupper(e->data.items[e->cursor]) && e->cursor != start_pos - 1) {
                break; // Break if it's an uppercase letter and not the first letter of the word
            }
        }
    } else {
        // If the character left of the cursor is not alphanumeric, move until an alphanumeric is found
        while (e->cursor > 0 && !isalnum(e->data.items[e->cursor - 1])) {
            e->cursor -= 1;
        }
    }

    size_t end_pos = e->cursor;

    if (start_pos > end_pos) {
        // Copy the deleted text to clipboard
        size_t length = start_pos - end_pos;
        e->clipboard.count = 0;
        sb_append_buf(&e->clipboard, &e->data.items[end_pos], length);
        sb_append_null(&e->clipboard);
        if (SDL_SetClipboardText(e->clipboard.items) < 0) {
            fprintf(stderr, "ERROR: SDL ERROR: %s\n", SDL_GetError());
        }

        // Perform the deletion
        memmove(&e->data.items[end_pos], &e->data.items[start_pos], e->data.count - start_pos);
        e->data.count -= length;
    }

    editor_retokenize(e);
}

void emacs_back_to_indentation(Editor *e) {
    if (e->cursor >= e->data.count) return;
    size_t row = editor_cursor_row(e);
    size_t line_begin = e->lines.items[row].begin;
    size_t line_end = e->lines.items[row].end;
    size_t first_non_whitespace = find_first_non_whitespace(e->data.items, line_begin, line_end);
    e->cursor = first_non_whitespace;
}

void emacs_mark_paragraph(Editor *e) {
    if (!e->selection) {
        // Find the first empty line above
        size_t row = editor_cursor_row(e);
        while (row > 0 && !editor_is_line_empty(e, row - 1)) {
            row--;
        }

        // Set the selection start to the beginning of the line after the empty line
        e->select_begin = e->lines.items[row].begin;
        e->cursor = e->select_begin;
        e->selection = true;
    }

    // Extend the selection downwards to the end of the paragraph
    editor_move_paragraph_down(e);
}
