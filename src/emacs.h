#ifndef EMACS_H
#define EMACS_H

#include "editor.h"

void emacs_kill_line(Editor *e);
void emacs_backward_kill_word(Editor *e);
void emacs_back_to_indentation(Editor *e);
void emacs_mark_paragraph(Editor *e);
void emacs_ungry_delete_backwards(Editor *e);

#endif // EMACS_H
