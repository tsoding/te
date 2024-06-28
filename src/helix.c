#include <stdlib.h>
#include <time.h>
#include "helix.h"
#include "editor.h"
#include "lexer.h"
#include "theme.h"

void helix_mode() {
    if (current_mode != HELIX) {
        current_mode = HELIX;
        switch_to_theme(&currentThemeIndex, 7);
        targetModelineHeight = 21.0f;
        targetMinibufferHeight = 0.0f;
    } else {
        current_mode = NORMAL;
        targetModelineHeight = 35.0f;
        targetMinibufferHeight = 21.0f;
        srand(time(NULL));

        int randomThemeIndex;
        do {
            randomThemeIndex = rand() % 8;
        } while (randomThemeIndex == 7); // Ensure the random theme is not Helix

        switch_to_theme(&currentThemeIndex, randomThemeIndex);
    }

    minibufferAnimationProgress = 0.0f;
    isModelineAnimating = true;
    isMinibufferAnimating = true;
}

void update_cursor_color(Editor *editor) {
    // Check for no text
    if (editor == NULL || editor->data.items == NULL || editor->data.count == 0) {
        currentTheme.cursor = themes[currentThemeIndex].notext_cursor;
        return;
    }

    size_t cursor_pos = editor->cursor;

    // check if cursor is at EOF
    if (cursor_pos >= editor->data.count) {
        currentTheme.cursor = themes[currentThemeIndex].EOF_cursor;
        return;
    }

    // Check if the cursor is on a whitespace
    if (isspace(editor->data.items[cursor_pos])) {
        currentTheme.cursor = themes[currentThemeIndex].cursor;
        return;
    }

    size_t current_pos = 0;
    size_t token_index = 0; // Current token being processed

    while (current_pos < editor->data.count && token_index < editor->tokens.count) {
        Token token = editor->tokens.items[token_index];
        size_t token_end = current_pos + token.text_len;

        // Check if the cursor is within the current token
        if (cursor_pos >= current_pos && cursor_pos < token_end) {
            Vec4f color = get_color_for_token_kind(token.kind);
            currentTheme.cursor = color;
            return;
        } else {
            currentTheme.cursor = currentTheme.text;
        }

        // Advance to the next token or character
        if (cursor_pos < token_end || strncmp(&editor->data.items[current_pos], token.text, token.text_len) == 0) {
            current_pos = token_end; // Skip over the token
            token_index++;           // Move to the next token
        } else {
            current_pos++; // Move to the next character
        }
    }
}




Vec4f get_color_for_token_kind(Token_Kind kind) {
    switch (kind) {
        case TOKEN_KEYWORD: return currentTheme.logic;
        case TOKEN_STRING: return currentTheme.string;
        case TOKEN_TYPE: return currentTheme.type;
        case TOKEN_PIPE: return currentTheme.pipe;
        case TOKEN_TRUE: return currentTheme.truee;
        case TOKEN_FALSE: return currentTheme.falsee;
        case TOKEN_NULL: return currentTheme.null;
        case TOKEN_PREPROC: return currentTheme.hashtag;
        case TOKEN_POINTER: return currentTheme.pointer;
        case TOKEN_EQUALS: return currentTheme.equals;
        case TOKEN_GREATER_THAN: return currentTheme.greater_than;
        case TOKEN_LESS_THAN: return currentTheme.less_than;
        case TOKEN_EQUALS_EQUALS: return currentTheme.equals_equals;
        case TOKEN_COMMENT: return currentTheme.comment;
        case TOKEN_ARROW: return currentTheme.arrow;
        case TOKEN_FUNCTION_DEFINITION: return currentTheme.function_definition;
        case TOKEN_ARRAY_CONTENT: return currentTheme.array_content;
        case TOKEN_OPEN_SQUARE: return currentTheme.open_square;
        case TOKEN_CLOSE_SQUARE: return currentTheme.close_square;
        case TOKEN_OPEN_CURLY: return currentTheme.open_curly;
        case TOKEN_CLOSE_CURLY: return currentTheme.close_curly;
        default: return currentTheme.cursor;
    }
}

