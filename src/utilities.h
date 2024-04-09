#ifndef UTILITIES_H
#define UTILITIES_H

#include "editor.h"

// UTILITY
bool extractLine(Editor *editor, size_t cursor, char *line, size_t max_length);
size_t editor_row_from_pos(const Editor *e, size_t pos);
bool extract_word_under_cursor(Editor *editor, char *word);
bool extract_word_left_of_cursor(Editor *e, char *word, size_t max_word_length);
bool editor_is_line_empty(Editor *e, size_t row);
bool editor_is_line_whitespaced(Editor *e, size_t row);
float measure_whitespace_width(Free_Glyph_Atlas *atlas);
float measure_whitespace_height(Free_Glyph_Atlas *atlas);
size_t find_first_non_whitespace(const char* items, size_t begin, size_t end);
bool is_number(const char *str);



#endif // UTILITIES_H
