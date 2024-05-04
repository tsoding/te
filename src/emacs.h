#ifndef EMACS_H
#define EMACS_H

#include "editor.h"

extern int killed_word_times;
void emacs_kill_word(Editor *e);

void emacs_kill_line(Editor *e);
void emacs_backward_kill_word(Editor *e);
void emacs_back_to_indentation(Editor *e);
void emacs_mark_paragraph(Editor *e);
void emacs_ungry_delete_backwards(Editor *e);
void emacs_open_line(Editor *e);
void emacs_mwim_beginning(Editor *e);
void emacs_mwim_end(Editor *e);
void emacs_delete_char(Editor *e);


#endif // EMACS_H
