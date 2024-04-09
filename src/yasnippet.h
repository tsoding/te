#ifndef YASNIPPET_H_
#define YASNIPPET_H_

#include "editor.h"

#define MAX_SNIPPET_KEY_LENGTH 50
#define MAX_SNIPPET_CONTENT_LENGTH 1024

typedef struct {
    char key[MAX_SNIPPET_KEY_LENGTH];
    char content[MAX_SNIPPET_CONTENT_LENGTH];
} Snippet;

typedef struct {
    Snippet *array;
    size_t used;
    size_t size;
} SnippetArray;

extern SnippetArray snippets;

void init_snippet_array(SnippetArray *a, size_t initial_size);
void insert_snippet(SnippetArray *a, Snippet snippet);
void free_snippet_array(SnippetArray *a);
void load_snippets_from_directory();
void activate_snippet(Editor *e);

#endif // YASNIPPET_H_
