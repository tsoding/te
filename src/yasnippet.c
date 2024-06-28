#include <dirent.h>
#include <errno.h>
#include "editor.h"
#include "yasnippet.h"
#include "utilities.h"

SnippetArray snippets;

void init_snippet_array(SnippetArray *a, size_t initial_size) {
    a->array = (Snippet *)malloc(initial_size * sizeof(Snippet));
    a->used = 0;
    a->size = initial_size;
}

void insert_snippet(SnippetArray *a, Snippet snippet) {
    if (a->used == a->size) {
        a->size *= 2;
        a->array = (Snippet *)realloc(a->array, a->size * sizeof(Snippet));
    }
    a->array[a->used++] = snippet;
}

void free_snippet_array(SnippetArray *a) {
    free(a->array);
    a->array = NULL;
    a->used = a->size = 0;
}


void load_snippets_from_directory() {
    const char * home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "ERROR: HOME environment variable not set.\n");
        return;
    }

    char directory[256];
    snprintf(directory, sizeof(directory), "%s/.config/ded/snippets", home);

    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(directory)) == NULL) {
        fprintf(stderr, "opendir failed: %s\n", strerror(errno));
        return;
    }

    init_snippet_array(&snippets, 10); // Start with an initial size of 10

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char filepath[256];
            snprintf(filepath, sizeof(filepath), "%s/%s", directory, entry->d_name);

            FILE *file = fopen(filepath, "r");
            if (file) {
                Snippet new_snippet;
                strncpy(new_snippet.key, entry->d_name, MAX_SNIPPET_KEY_LENGTH - 1);
                new_snippet.key[MAX_SNIPPET_KEY_LENGTH - 1] = '\0';
                new_snippet.content[0] = '\0'; // Initialize content as empty string

                char line[256];
                while (fgets(line, sizeof(line), file)) {
                    strncat(new_snippet.content, line, MAX_SNIPPET_CONTENT_LENGTH - strlen(new_snippet.content) - 1);
                }
                new_snippet.content[MAX_SNIPPET_CONTENT_LENGTH - 1] = '\0';

                insert_snippet(&snippets, new_snippet);
                fclose(file);
            }
        }
    }

    closedir(dir);
}

// TODO Indentation problem
void activate_snippet(Editor *e) {
    char word[MAX_SNIPPET_KEY_LENGTH];
    size_t original_cursor_position = e->cursor;

    if (!extract_word_left_of_cursor(e, word, sizeof(word))) {
        indent(e);
        return;
    }

    bool snippet_found = false; // Flag to check if a snippet is found

    for (size_t i = 0; i < snippets.used; i++) {
        if (strcmp(snippets.array[i].key, word) == 0) {
            snippet_found = true; // A matching snippet is found.
            size_t word_length = strlen(word);

            // Delete the word from the buffer
            memmove(&e->data.items[e->cursor],
                    &e->data.items[e->cursor + word_length],
                    e->data.count - (e->cursor + word_length));
            e->data.count -= word_length;

            // Duplicate snippet content to manipulate
            char *snippet_content = strdup(snippets.array[i].content);
            char *placeholder_pos = strstr(snippet_content, "$0");

            // Capture the current indentation level
            size_t cursor_row = editor_row_from_pos(e, e->cursor);
            size_t line_start = e->lines.items[cursor_row].begin;
            size_t current_indent = e->cursor - line_start;

            // Calculate the position of $0
            size_t placeholder_line = 0;
            size_t placeholder_col = 0;
            if (placeholder_pos) {
                for (char *p = snippet_content; p < placeholder_pos; ++p) {
                    if (*p == '\n') {
                        placeholder_line++;
                        placeholder_col = 0;
                    } else {
                        placeholder_col++;
                    }
                }
                memmove(placeholder_pos, placeholder_pos + 2, strlen(placeholder_pos + 2) + 1);  // Remove $0
            }

            // Process each line of the snippet
            char *line_start_ptr = snippet_content;
            char *line_end_ptr;
            while ((line_end_ptr = strchr(line_start_ptr, '\n')) != NULL || *line_start_ptr) {
                if (line_end_ptr != NULL) {
                    size_t line_length = line_end_ptr - line_start_ptr;
                    if (line_length > 0) {
                        editor_insert_buf(e, line_start_ptr, line_length);
                    }
                    editor_insert_char(e, '\n'); // Insert newline and move to the next line
                    line_start_ptr = line_end_ptr + 1;
                } else {
                    // Last line of the snippet
                    editor_insert_buf(e, line_start_ptr, strlen(line_start_ptr));
                    break;
                }

                // Apply indentation for new lines
                if (*line_start_ptr && cursor_row != editor_row_from_pos(e, e->cursor)) {
                    for (size_t i = 0; i < current_indent; ++i) {
                        editor_insert_char(e, ' ');
                    }
                }
            }

            // Adjust cursor position to where $0 was
            if (placeholder_pos) {
                e->cursor = e->lines.items[cursor_row + placeholder_line].begin + placeholder_col + (placeholder_line > 0 ? current_indent : 0);
            }

            free(snippet_content);
            break; // Exit the loop as the snippet is found and processed.
        }
    }

    if (!snippet_found) {
        e->cursor = original_cursor_position; // Restore cursor to its original position.
        indent(e);
    }
}




// THIS fixes it but also bring new problems

/* void activate_snippet(Editor *e) { */
/*     char word[MAX_SNIPPET_KEY_LENGTH]; */
/*     size_t original_cursor_position = e->cursor; */

/*     // Extract the word left of the cursor to identify the snippet to activate */
/*     if (!extract_word_left_of_cursor(e, word, sizeof(word))) { */
/*         indent(e); // If no word is found, simply indent the line */
/*         return; */
/*     } */

/*     bool snippet_found = false; */

/*     for (size_t i = 0; i < snippets.used; i++) { */
/*         if (strcmp(snippets.array[i].key, word) == 0) { */
/*             snippet_found = true; */

/*             // Delete the word triggering the snippet */
/*             size_t word_length = strlen(word); */
/*             memmove(&e->data.items[e->cursor], */
/*                     &e->data.items[e->cursor + word_length], */
/*                     e->data.count - (e->cursor + word_length)); */
/*             e->data.count -= word_length; */

/*             char *snippet_content = strdup(snippets.array[i].content); */
/*             char *placeholder_pos = strstr(snippet_content, "$0"); */

/*             // Capture the initial indentation level */
/*             size_t cursor_row = editor_row_from_pos(e, e->cursor); */
/*             size_t line_start = e->lines.items[cursor_row].begin; */
/*             size_t current_indent = e->cursor - line_start; */

/*             // Process placeholder and snippet content */
/*             size_t placeholder_line = 0, placeholder_col = 0; */
/*             if (placeholder_pos) { */
/*                 for (char *p = snippet_content; p < placeholder_pos; ++p) { */
/*                     if (*p == '\n') { */
/*                         placeholder_line++; */
/*                         placeholder_col = 0; */
/*                     } else { */
/*                         placeholder_col++; */
/*                     } */
/*                 } */
/*                 // Remove $0 from the content */
/*                 memmove(placeholder_pos, placeholder_pos + 2, strlen(placeholder_pos + 2) + 1); */
/*             } */

/*             // Insert the snippet content */
/*             char *line_start_ptr = snippet_content; */
/*             char *line_end_ptr; */
/*             while ((line_end_ptr = strchr(line_start_ptr, '\n')) != NULL || *line_start_ptr) { */
/*                 if (line_end_ptr) { */
/*                     editor_insert_buf(e, line_start_ptr, line_end_ptr - line_start_ptr); */
/*                     editor_insert_char(e, '\n'); */
/*                     line_start_ptr = line_end_ptr + 1; */
/*                 } else { */
/*                     editor_insert_buf(e, line_start_ptr, strlen(line_start_ptr)); */
/*                     break; */
/*                 } */

/*                 // Apply initial indentation to new lines */
/*                 if (*line_start_ptr && cursor_row != editor_row_from_pos(e, e->cursor)) { */
/*                     for (size_t i = 0; i < current_indent; ++i) { */
/*                         editor_insert_char(e, ' '); */
/*                     } */
/*                 } */
/*             } */

/*             // Adjust the cursor to the position of $0, considering initial indentation */
/*             if (placeholder_pos) { */
/*                 // Calculate new cursor position */
/*                 e->cursor = original_cursor_position - word_length; */
/*                 for (size_t i = 0; i <= placeholder_line && i < e->lines.count; ++i) { */
/*                     e->cursor += (i < placeholder_line ? e->lines.items[cursor_row + i].end - e->lines.items[cursor_row + i].begin + 1 : placeholder_col); */
/*                     // Apply initial indentation if we're past the first line of the snippet */
/*                     if (i > 0) { */
/*                         e->cursor += current_indent; */
/*                     } */
/*                 } */
/*             } */

/*             free(snippet_content); */
/*             break; // Found the snippet, no need to continue searching */
/*         } */
/*     } */

/*     // If the snippet wasn't found, restore the cursor position and indent */
/*     if (!snippet_found) { */
/*         e->cursor = original_cursor_position; */
/*         indent(e); */
/*     } */
/* } */





