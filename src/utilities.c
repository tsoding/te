#include "utilities.h"

// Utility functions make it easier to add new functionality

bool extractLine(Editor *editor, size_t cursor, char *line, size_t max_length) {
    size_t start = cursor;
    while (start > 0 && editor->data.items[start - 1] != '\n') {
        start--;
    }

    size_t end = start;
    while (end < editor->data.count && editor->data.items[end] != '\n') {
        end++;
    }

    size_t length = end - start;
    if (length < max_length) {
        strncpy(line, &editor->data.items[start], length);
        line[length] = '\0';
        return true;
    }

    return false;
}

size_t editor_row_from_pos(const Editor *e, size_t pos) {
    assert(e->lines.count > 0);
    for (size_t row = 0; row < e->lines.count; ++row) {
        Line line = e->lines.items[row];
        if (line.begin <= pos && pos <= line.end) {
            return row;
        }
    }
    return e->lines.count - 1;
}

bool extract_word_under_cursor(Editor *editor, char *word) {
    size_t cursor = editor->cursor;

    // Move left to find the start of the word.
    while (cursor > 0 && isalnum(editor->data.items[cursor - 1])) {
        cursor--;
    }

    // Check if the cursor is on a word or on whitespace/special character.
    if (!isalnum(editor->data.items[cursor])) return false;

    int start = cursor;

    // Move right to find the end of the word.
    while (cursor < editor->data.count && isalnum(editor->data.items[cursor])) {
        cursor++;
    }

    int end = cursor;

    // Copy the word to the provided buffer.
    // Make sure not to overflow the buffer and null-terminate the string.
    int length = end - start;
    strncpy(word, &editor->data.items[start], length);
    word[length] = '\0';

    return true;
}

bool extract_word_left_of_cursor(Editor *e, char *word, size_t max_word_length) {
    if (e->cursor == 0 || !isalnum(e->data.items[e->cursor - 1])) {
        return false;
    }

    size_t end = e->cursor;
    size_t start = end;

    while (start > 0 && isalnum(e->data.items[start - 1])) {
        start--;
    }

    size_t word_length = end - start;
    if (word_length >= max_word_length) {
        return false;
    }

    memcpy(word, &e->data.items[start], word_length);
    word[word_length] = '\0';
    e->cursor = start;
    return true;
}

bool editor_is_line_empty(Editor *e, size_t row) {
    if (row >= e->lines.count) return true; // Non-existent lines are considered empty

    return e->lines.items[row].begin == e->lines.items[row].end;
}

bool editor_is_line_whitespaced(Editor *e, size_t row) {
    if (row >= e->lines.count) return false;

    size_t line_begin = e->lines.items[row].begin;
    size_t line_end = e->lines.items[row].end;

    for (size_t i = line_begin; i < line_end; ++i) {
        if (!isspace(e->data.items[i])) {
            return false;
        }
    }
    return true;
}

float measure_whitespace_width(Free_Glyph_Atlas *atlas) {
    Vec2f whitespaceSize = {0.0f, 0.0f};
    free_glyph_atlas_measure_line_sized(atlas, " ", 1, &whitespaceSize);
    return whitespaceSize.x;
}

float measure_whitespace_height(Free_Glyph_Atlas *atlas) {
    Vec2f whitespaceSize = {0.0f, 0.0f};
    free_glyph_atlas_measure_line_sized(atlas, " ", 1, &whitespaceSize);
    return whitespaceSize.y;
}

size_t find_first_non_whitespace(const char* items, size_t begin, size_t end) {
    size_t pos = begin;
    while (pos < end && isspace((unsigned char)items[pos])) {
        pos++;
    }
    return pos;
}

size_t find_last_non_whitespace(const char* items, size_t begin, size_t end) {
    if (end > begin) {
        size_t pos = end;
        do {
            pos--;
            if (!isspace((unsigned char)items[pos])) {
                return pos + 1; // return the position right after the non-whitespace char
            }
        } while (pos > begin);
    }
    return begin; // If no non-whitespace found, return the beginning (handles the empty line case too)
}



bool is_number(const char *str) {
    if (!str || *str == '\0')
        return false;  // Empty string is not a number

    // Check if each character is a digit
    for (const char *p = str; *p != '\0'; p++) {
        if (!isdigit((unsigned char)*p))
            return false;
    }
    return true;
}

