#ifndef HELIX_H
#define HELIX_H

#include "editor.h"

void helix_mode();
void update_cursor_color(Editor *editor);
Vec4f get_color_for_token_kind(Token_Kind kind);

#endif // HELIX_H
