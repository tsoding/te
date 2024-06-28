#ifndef BUFFER_H
#define BUFFER_H

#include "editor.h"

// BUFFER
void editor_add_to_buffer_history(Editor *e, const char *file_path);
void editor_remove_from_buffer_history(Editor *e);
Errno editor_open_buffer(Editor *e, const char *file_path);
Errno editor_open_buffer(Editor *e, const char *file_path);
void editor_kill_buffer(Editor *e);
void editor_previous_buffer(Editor *e);
void editor_next_buffer(Editor *e);


#endif // BUFFER_H
