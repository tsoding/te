#include "render.h"
#include "editor.h"
#include "file_browser.h"
#include "free_glyph.h"
#include "la.h"
#include "lexer.h"
#include "simple_renderer.h"
#include "theme.h"
#include <stdbool.h>
#include <time.h>
#include "clock.h"
#include "utilities.h"

float lineNumberWidth = FREE_GLYPH_FONT_SIZE * 1;
bool render_whitespaces_on_select = false;
bool lerpTokens = true; // TODO
bool mixSelectionColor = false;


Vec4f blend_color(Vec4f color1, Vec4f color2, float blendFactor) {
    Vec4f result;
    result.x = color1.x * (1 - blendFactor) + color2.x * blendFactor;
    result.y = color1.y * (1 - blendFactor) + color2.y * blendFactor;
    result.z = color1.z * (1 - blendFactor) + color2.z * blendFactor;
    result.w = color1.w * (1 - blendFactor) + color2.w * blendFactor;
    return result;
}


void render_fill_column(Simple_Renderer *sr, Free_Glyph_Atlas *atlas, Editor *editor) {
    float len = calculate_max_line_length(editor);
    if (smartFillColumn && len < fillColumn + 1 || showFillColumn == false) {
        return;
    }

    float characterWidth = measure_whitespace_width(atlas);
    float columnPosition = characterWidth * fillColumn;

    if (showLineNumbers) {
        columnPosition += lineNumberWidth;
    }

    Vec2f startPos = {columnPosition, sr->camera_pos.y - (sr->resolution.y / 2) / sr->camera_scale};
    Vec2f endPos = {columnPosition, sr->camera_pos.y + (sr->resolution.y / 2) / sr->camera_scale};

    float COLUMN_THICKNESS;
    if (fillColumnThickness == 0) {
        COLUMN_THICKNESS = characterWidth;
    } else {
        COLUMN_THICKNESS = fillColumnThickness;
    }

    Vec4f COLUMN_COLOR = CURRENT_THEME.fill_column;
    simple_renderer_solid_rect(sr, startPos, vec2f(COLUMN_THICKNESS, endPos.y - startPos.y), COLUMN_COLOR);

    simple_renderer_flush(sr);
}

// TODO
void render_search_text(Free_Glyph_Atlas *atlas, Simple_Renderer *sr, Editor *editor) {
    if (editor->searching) {
        Vec4f cursorColor = CURRENT_THEME.cursor;
        Vec4f textColor = CURRENT_THEME.text;
        Vec2f searchPos = {30.0f, 20.0f};
        float minibufferCursorOffsett = 5.0f;

        // Render the search text
        simple_renderer_set_shader(sr, VERTEX_SHADER_MINIBUFFER, SHADER_FOR_TEXT);
        free_glyph_atlas_render_line_sized(atlas, sr, editor->search.items, editor->search.count, &searchPos, textColor);

        // Set cursor position at the start of the text (we already used those we can change them)
        searchPos.y = 0.0f;
        searchPos.x += minibufferCursorOffsett;
        Vec2f cursorPos = searchPos;

        // Set cursor size
        float cursor_width = measure_whitespace_width(atlas);
        Vec2f cursorSize = {cursor_width, minibufferHeight * 4.0f};

        // Render the cursor
        simple_renderer_flush(sr);
        simple_renderer_set_shader(sr, VERTEX_SHADER_MINIBUFFER, SHADER_FOR_COLOR);
        simple_renderer_solid_rect(sr, cursorPos, cursorSize, cursorColor);

        // Flush the renderer
        simple_renderer_flush(sr);
    }
}

void render_minibuffer_content(Free_Glyph_Atlas *atlas, Simple_Renderer *sr, Editor *editor, const char *prefixText) {
    if (editor->minibuffer_active) {
        Vec4f cursorColor = CURRENT_THEME.cursor;
        Vec4f textColor = CURRENT_THEME.text;
        Vec2f searchPos = {30.0f, 20.0f};
        float prefixRightPadding;
        float minibufferCursorOffsett = 5.0f;

        if (M_x_active) {
            prefixRightPadding = 50;
        } else if (evil_command_active) {
            prefixRightPadding = 0;
        } else if (editor->searching) {
            prefixRightPadding = 0;
        }

        if (editor->searching) {
            // TODO
        } else {
            // Render the prefix
            free_glyph_atlas_render_line_sized(atlas, sr, prefixText, strlen(prefixText), &searchPos, cursorColor);
            
            // Calculate the width of the prefix and adjust the position for minibuffer text
            float prefixWidth = free_glyph_atlas_measure_line_width(atlas, prefixText, strlen(prefixText));
            searchPos.x += prefixRightPadding;
            
            // Render the minibuffer text
            simple_renderer_set_shader(sr, VERTEX_SHADER_MINIBUFFER, SHADER_FOR_TEXT);
            free_glyph_atlas_render_line_sized(atlas, sr, editor->minibuffer_text.items, editor->minibuffer_text.count, &searchPos, textColor);
            
            // Adjust cursor position according to your original logic
            searchPos.x += minibufferCursorOffsett; // Adjust x for the cursor
            searchPos.y = 0.0f; // Reset y for the cursor
            Vec2f cursorPos = searchPos;
            
            // Set cursor size
            float cursor_width = measure_whitespace_width(atlas);
            Vec2f cursorSize = {cursor_width, 21.0f * 4.0f}; // 21 is the minibufferHeight
            
            // Render the cursor
            simple_renderer_flush(sr);
            simple_renderer_set_shader(sr, VERTEX_SHADER_MINIBUFFER, SHADER_FOR_COLOR);
            simple_renderer_solid_rect(sr, cursorPos, cursorSize, cursorColor);
        }
        // Flush the renderer
        simple_renderer_flush(sr);
    }
}




void render_selection(Editor *editor, Simple_Renderer *sr, Free_Glyph_Atlas *atlas) {
    if (isWave) {
        simple_renderer_set_shader(sr, VERTEX_SHADER_WAVE, SHADER_FOR_COLOR);
    } else if (editor->selection) {
        simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_CURSOR);
    } else {
        simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR);
    }

    if (editor->selection) {
        Vec4f selection_color;
        if (mixSelectionColor) {
            selection_color = blend_color(currentTheme.cursor, currentTheme.selection, 0.5);
        } else {
            selection_color = themes[currentThemeIndex].selection;
        }

        for (size_t row = 0; row < editor->lines.count; ++row) {
            size_t select_begin_chr = editor->select_begin;
            size_t select_end_chr = editor->cursor;
            if (select_begin_chr > select_end_chr) {
                SWAP(size_t, select_begin_chr, select_end_chr);
            }

            Line line_chr = editor->lines.items[row];

            if (select_begin_chr < line_chr.begin) {
                select_begin_chr = line_chr.begin;
            }

            if (select_end_chr > line_chr.end) {
                select_end_chr = line_chr.end;
            }

            if (select_begin_chr <= select_end_chr) {
                Vec2f select_begin_scr = vec2f(0, -((float)row + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE);
                free_glyph_atlas_measure_line_sized(atlas, editor->data.items + line_chr.begin, select_begin_chr - line_chr.begin, &select_begin_scr);

                Vec2f select_end_scr = select_begin_scr;
                // Adjust the range to include the end character
                free_glyph_atlas_measure_line_sized(atlas, editor->data.items + select_begin_chr, select_end_chr - select_begin_chr + 1, &select_end_scr);

                // Adjust selection for line numbers if displayed
                if (showLineNumbers) {
                    select_begin_scr.x += lineNumberWidth;
                    select_end_scr.x += lineNumberWidth;
                }

                simple_renderer_solid_rect(sr, select_begin_scr, vec2f(select_end_scr.x - select_begin_scr.x, FREE_GLYPH_FONT_SIZE), selection_color);
            }
        }
    }
    simple_renderer_flush(sr);
}




#include <string.h> // Include string.h for strcmp

typedef struct {
    size_t startLine;
    size_t endLine;
    float startX;
} MarkdownCodeBlockInfo;


// TODO allign codeblock with cursor and make them expandable adding chars
void render_markdown(Free_Glyph_Atlas *atlas, Simple_Renderer *sr, Editor *editor, File_Browser *fb) {
    const float LINE_HEIGHT = FREE_GLYPH_FONT_SIZE;
    MarkdownCodeBlockInfo codeBlockStack[500]; // Assuming a max of 500 code blocks
    int codeBlockCount = 0;

    for (size_t i = 0; i < editor->lines.count; ++i) {
        Line line = editor->lines.items[i];
        float lineStartX = 0; // Start of the line

        // Check for code block start or end
        if (line.end - line.begin >= 3 && strncmp(editor->data.items + line.begin, "```", 3) == 0) {
            if (codeBlockCount > 0 && codeBlockStack[codeBlockCount - 1].endLine == 0) {
                // Closing code block
                codeBlockStack[codeBlockCount - 1].endLine = i;
            } else {
                // Starting new code block
                codeBlockStack[codeBlockCount++] = (MarkdownCodeBlockInfo){i, 0, lineStartX};
            }
        }
    }

    // Draw rectangles for each code block
    for (int k = 0; k < codeBlockCount; k++) {
        if (codeBlockStack[k].endLine > 0) { // Only if code block is closed
            // Start one line before
            Vec2f startPos = {codeBlockStack[k].startX, -((float)codeBlockStack[k].startLine + CURSOR_OFFSET - 1) * LINE_HEIGHT};
            // End one line later
            Vec2f endPos = {1000.0f, -((float)codeBlockStack[k].endLine + CURSOR_OFFSET) * LINE_HEIGHT}; // Removed the -1


            if (showLineNumbers) {
                startPos.x += lineNumberWidth;
                endPos.x += lineNumberWidth;
            }
            
            Vec4f codeBlockColor = CURRENT_THEME.code_block;
            Vec2f rectSize = {/* endPos.x - */ startPos.x + 2000.0f, endPos.y - startPos.y}; // TODO use w

            simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR);
            simple_renderer_solid_rect(sr, startPos, rectSize, codeBlockColor);
        }
    }
}

void render_line_numbers(Simple_Renderer *sr, Free_Glyph_Atlas *atlas, Editor *editor) {
    if (showLineNumbers) {
        simple_renderer_set_shader(sr, isWave ? VERTEX_SHADER_WAVE : VERTEX_SHADER_SIMPLE, SHADER_FOR_TEXT);

        adjust_line_number_width(editor, &lineNumberWidth);
        
        size_t currentLineNumber = editor_cursor_row(editor);

        Vec4f defaultColor = CURRENT_THEME.line_numbers;
        Vec4f currentLineColor = CURRENT_THEME.current_line_number;

        if (highlightCurrentLineNumberOnInsertMode) {
            currentLineColor = (current_mode == INSERT) ? CURRENT_THEME.insert_cursor :
                               (current_mode == EMACS) ? CURRENT_THEME.emacs_cursor :
                               CURRENT_THEME.current_line_number;
        }

        int lineNumberFieldWidth;
        size_t lineCount = editor->lines.count;

        // Adjust line number width based on followCursor and line count
        if (followCursor || lineCount >= 1000) {
            // Adjust width based on current line count
            lineNumberFieldWidth = snprintf(NULL, 0, "%zu", lineCount);
        } else {
            // Fixed width for up to three digits
            lineNumberFieldWidth = snprintf(NULL, 0, "%d", 999);
        }

        for (size_t i = 0; i < lineCount; ++i) {
            char lineNumberStr[12]; 
            
            size_t displayLineNumber = relativeLineNumbers ?
                (i == currentLineNumber) ? currentLineNumber + 1 : abs((int)i - (int)currentLineNumber) :
                i + 1;

            snprintf(lineNumberStr, sizeof(lineNumberStr), "%*zu", lineNumberFieldWidth, displayLineNumber);
            
            Vec2f pos = {0, -((float)i + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE};

            Vec4f colorToUse = (highlightCurrentLineNumber && i == currentLineNumber) ? currentLineColor : defaultColor;
            
            free_glyph_atlas_render_line_sized(atlas, sr, lineNumberStr, strlen(lineNumberStr), &pos, colorToUse);
        }

        simple_renderer_flush(sr);
    }
}

void adjust_line_number_width(Editor *editor, float *lineNumberWidth) {
    size_t lineCount = editor->lines.count;
    
    if (lineCount < 10) {          // Less than 10 lines
        *lineNumberWidth = FREE_GLYPH_FONT_SIZE * 3;
    } else if (lineCount < 100) {  // 10 to 99 lines
        *lineNumberWidth = FREE_GLYPH_FONT_SIZE * 3;
    } else if (lineCount < 1000) { // 100 to 999 lines
        *lineNumberWidth = FREE_GLYPH_FONT_SIZE * 3;
    } else {                       // 1000 lines or more
        *lineNumberWidth = FREE_GLYPH_FONT_SIZE * 4;
    }
}

// TODO 
/* void render_whitespaces(Free_Glyph_Atlas *atlas, Simple_Renderer *sr, Editor *editor) { */
/*     float circleRadius = FREE_GLYPH_FONT_SIZE * 0.1; */
/*     Vec4f whitespaceColor = CURRENT_THEME.whitespace; */
/*     int circleSegments = 20; */

/*     for (size_t i = 0; i < editor->lines.count; ++i) { */
/*         Line line = editor->lines.items[i]; */
/*         Vec2f pos = {0, -((float)i + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE}; */

/*         if (showLineNumbers) { */
/*             pos.x += lineNumberWidth; */
/*         } */

/*         // Manually calculate the selection start and end */
/*         size_t selectionStart = editor->select_begin; */
/*         size_t selectionEnd = editor->cursor; */
/*         if (selectionStart > selectionEnd) { */
/*             size_t temp = selectionStart; */
/*             selectionStart = selectionEnd; */
/*             selectionEnd = temp; */
/*         } */

/*         for (size_t j = line.begin; j < line.end; ++j) { */
/*             bool isWhitespace = editor->data.items[j] == ' ' || editor->data.items[j] == '\t'; */
/*             bool isInSelection = editor->selection && j >= selectionStart && j < selectionEnd; */
/*             bool shouldRenderAll = showWhitespaces && isWhitespace; */
/*             bool shouldRenderInSelection = render_whitespaces_on_select && isInSelection && isWhitespace; */

/*             if (shouldRenderAll || shouldRenderInSelection) { */
/*                 Vec2f char_pos = pos; */
/*                 char_pos.x += (j - line.begin) * circleRadius * 2; */
/*                 free_glyph_atlas_measure_line_sized(atlas, editor->data.items + j, 1, &char_pos); */
/*                 float char_width = char_pos.x - pos.x - (j - line.begin) * circleRadius * 2; */

/*                 Vec2f circleCenter = {pos.x + (j - line.begin) * char_width + char_width / 2, pos.y + FREE_GLYPH_FONT_SIZE / 2}; */

/*                 simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR); */
/*                 simple_renderer_circle(sr, circleCenter, circleRadius, whitespaceColor, circleSegments); */
/*             } */
/*         } */
/*     } */

/*     simple_renderer_flush(sr); */
/* } */

void render_whitespaces(Free_Glyph_Atlas *atlas, Simple_Renderer *sr, Editor *editor) {
    float circleRadius = FREE_GLYPH_FONT_SIZE * 0.1;
    int circleSegments = 20;

    for (size_t i = 0; i < editor->lines.count; ++i) {
        Line line = editor->lines.items[i];
        Vec2f pos = {0, -((float)i + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE};

        if (showLineNumbers) {
            pos.x += lineNumberWidth;
        }

        // Manually calculate the selection start and end
        size_t selectionStart = editor->select_begin;
        size_t selectionEnd = editor->cursor;
        bool isSelectingLeftToRight = selectionStart <= editor->cursor;
        if (selectionStart > selectionEnd) {
            size_t temp = selectionStart;
            selectionStart = selectionEnd;
            selectionEnd = temp;
        }

        for (size_t j = line.begin; j < line.end; ++j) {
            bool isWhitespace = editor->data.items[j] == ' ' || editor->data.items[j] == '\t';
            bool isInSelection = editor->selection && j >= selectionStart && j < selectionEnd;
            bool shouldRenderAll = showWhitespaces && isWhitespace;
            bool shouldRenderInSelection = render_whitespaces_on_select && isInSelection && isWhitespace;

            // Skip rendering whitespace at cursor's original position when selecting left to right
            if (isSelectingLeftToRight && j == editor->cursor) {
                continue;
            }

            if (shouldRenderAll || shouldRenderInSelection) {
                Vec2f char_pos = pos;
                char_pos.x += (j - line.begin) * circleRadius * 2;
                free_glyph_atlas_measure_line_sized(atlas, editor->data.items + j, 1, &char_pos);
                float char_width = char_pos.x - pos.x - (j - line.begin) * circleRadius * 2;

                Vec2f circleCenter = {pos.x + (j - line.begin) * char_width + char_width / 2, pos.y + FREE_GLYPH_FONT_SIZE / 2};

                Vec4f whitespaceColor = shouldRenderInSelection ? CURRENT_THEME.selected_whitespaces : CURRENT_THEME.whitespace;
                
                simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR);
                simple_renderer_circle(sr, circleCenter, circleRadius, whitespaceColor, circleSegments);
            }
        }
    }

    simple_renderer_flush(sr);
}


void render_indentation_lines(Simple_Renderer *sr, Free_Glyph_Atlas *atlas, Editor *editor) {
    if (!showIndentationLines) return;

    if (isWave) {
        simple_renderer_set_shader(sr, VERTEX_SHADER_WAVE, SHADER_FOR_COLOR);
    } else {
        simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR);
    }

    
    float COLUMN_THICKNESS = 5.0f;
    float CHARACTER_WIDTH = measure_whitespace_width(atlas);
    // This structure keeps track of open braces and their line start positions.
    struct LineSpan {
        size_t startLine;
        size_t endLine;
        float startX;
    } lineSpans[5000];
    int spanCount = 0;

    // Iterate through each line in the editor
    for (size_t i = 0; i < editor->lines.count; ++i) {
        Line line = editor->lines.items[i];

        // Iterate through each character in the line
        for (size_t j = line.begin; j < line.end; ++j) {
            char c = editor->data.items[j];
            if (c == '{') {
                ssize_t matchingPos = find_matching_parenthesis(editor, j);
                if (matchingPos != -1) {
                    size_t matchingLine = editor_row_from_pos(editor, matchingPos);

                    // Calculate the position of the first non-whitespace character
                    size_t firstNonWhitespace = find_first_non_whitespace(editor->data.items, line.begin, j);

                    // Calculate the X position where the line should start
                    float lineStartX = (firstNonWhitespace - line.begin) * CHARACTER_WIDTH;
                    if (showLineNumbers) {
                        lineStartX += lineNumberWidth;
                    }

                    // Add a new line span for the opening brace
                    lineSpans[spanCount++] = (struct LineSpan){i, matchingLine, lineStartX};
                }
            }
        }
    }

    // Draw lines for each collected brace pair
    for (int k = 0; k < spanCount; k++) {
        Vec2f startPos = {lineSpans[k].startX, -((float)lineSpans[k].startLine + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE};
        // Adjust endPos to make the line one line shorter, so it doesn't extend to the closing brace line
        Vec2f endPos = {startPos.x, -((float)lineSpans[k].endLine - 1 + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE};

        simple_renderer_solid_rect(sr, startPos, vec2f(COLUMN_THICKNESS, endPos.y - startPos.y), CURRENT_THEME.indentation_line);
    }
}





void editor_render(SDL_Window *window, Free_Glyph_Atlas *atlas, Simple_Renderer *sr, Editor *editor)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    float max_line_len = 0.0f;

    sr->resolution = vec2f(w, h);
    sr->time = (float) SDL_GetTicks() / 1000.0f;

    float whitespace_width = measure_whitespace_width(atlas);

    
    // Render hl_line
    {
        if (hl_line){
            simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR);
            
            size_t currentLine = editor_cursor_row(editor);
            Vec2f highlightPos = {0.0f, -((float)currentLine + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE};
            
            float highlightWidth = 8000;  // Default width for the highlight
            
            // If showing line numbers, adjust the position and width of the highlight
            if (showLineNumbers) {
                highlightPos.x -= lineNumberWidth - 260;  // Move highlight to the left to cover line numbers
                highlightWidth += lineNumberWidth;  // Increase width to include line numbers area
            }
            
            simple_renderer_solid_rect(sr, highlightPos, vec2f(highlightWidth, FREE_GLYPH_FONT_SIZE), CURRENT_THEME.hl_line);
            
            simple_renderer_flush(sr);
        }
    }

    // Render anchor
    if (editor->has_anchor) {
        simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR);

        // Update the anchor position before rendering
        editor_update_anchor(editor);
        
        size_t anchor_row = editor_row_from_pos(editor, editor->anchor_pos);
        Line anchor_line = editor->lines.items[anchor_row];
        size_t anchor_col = editor->anchor_pos - anchor_line.begin;
        
        Vec2f anchor_pos_vec = vec2fs(0.0f);
        anchor_pos_vec.y = -((float)anchor_row + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE;
        anchor_pos_vec.x = free_glyph_atlas_cursor_pos(
                                                       atlas,
                                                       editor->data.items + anchor_line.begin, anchor_line.end - anchor_line.begin,
                                                       vec2f(0.0, anchor_pos_vec.y),
                                                       anchor_col
                                                       );
        
        // Adjust anchor position if line numbers are shown
        if (showLineNumbers) {
            anchor_pos_vec.x += lineNumberWidth;
        }
        
        Vec4f ANCHOR_COLOR = CURRENT_THEME.anchor;
        
        simple_renderer_solid_rect(
                                   sr, anchor_pos_vec, vec2f(whitespace_width, FREE_GLYPH_FONT_SIZE),
                                   ANCHOR_COLOR);
        
        
        simple_renderer_flush(sr);
    }


    // TODO shader switch
    render_indentation_lines(sr, atlas, editor);
    render_selection(editor, sr, atlas);
    render_whitespaces(atlas, sr, editor);

    
    Vec2f cursor_pos = vec2fs(0.0f);
    {
        size_t cursor_row = editor_cursor_row(editor);
        Line line = editor->lines.items[cursor_row];
        size_t cursor_col = editor->cursor - line.begin;
        cursor_pos.y = -((float)cursor_row + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE;
        cursor_pos.x = free_glyph_atlas_cursor_pos(
                           atlas,
                           editor->data.items + line.begin, line.end - line.begin,
                           vec2f(0.0, cursor_pos.y),
                           cursor_col
                       );
    }

    
    // Render search
    {
        if (editor->searching) {
            simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR);
            Vec4f selection_color = CURRENT_THEME.search; // or .selection_color if that's what you named it in the struct.

            Vec2f p1 = cursor_pos;
            Vec2f p2 = p1;

            free_glyph_atlas_measure_line_sized(editor->atlas, editor->search.items, editor->search.count, &p2);

            // Adjust for line numbers width if they are displayed
            if (showLineNumbers) {
                p1.x += lineNumberWidth;
                p2.x += lineNumberWidth;
            }

            simple_renderer_solid_rect(sr, p1, vec2f(p2.x - p1.x, FREE_GLYPH_FONT_SIZE), selection_color);
            simple_renderer_flush(sr);
        }
    }

  
    render_line_numbers(sr, atlas, editor);

    // Render matching parenthesis
    {
        if (current_mode == NORMAL || current_mode == EMACS) {
            if (matchParenthesis) {
                if (isWave) {
                    simple_renderer_set_shader(sr, VERTEX_SHADER_WAVE, SHADER_FOR_COLOR);
                } else {
                    simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR);
                }
                
                ssize_t matching_pos = find_matching_parenthesis(editor, editor->cursor);
                if (matching_pos != -1) {
                    size_t matching_row = editor_row_from_pos(editor, matching_pos);
                    
                    Vec2f match_pos_screen = vec2fs(0.0f); // Initialize to zero
                    match_pos_screen.y = -((float)matching_row + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE;
                    
                    Line line = editor->lines.items[matching_row];
                    if (matching_pos >= line.begin && matching_pos < line.end) {
                        // Measure the position up to the matching character
                        free_glyph_atlas_measure_line_sized(atlas, editor->data.items + line.begin, matching_pos - line.begin, &match_pos_screen);
                        
                        // Measure the width of the actual character at the matching position
                        Vec2f char_end_pos = match_pos_screen;
                        free_glyph_atlas_measure_line_sized(atlas, editor->data.items + matching_pos, 1, &char_end_pos);
                        float char_width = char_end_pos.x - match_pos_screen.x;
                        
                        // Adjust for line numbers if displayed
                        if (showLineNumbers) {
                            match_pos_screen.x += lineNumberWidth;
                        }
                        
                        // Define the size of the highlight rectangle to match character size
                        Vec2f rect_size = vec2f(char_width, FREE_GLYPH_FONT_SIZE);
                        
                        simple_renderer_solid_rect(sr, match_pos_screen, rect_size, CURRENT_THEME.matching_parenthesis);
                    }
                }
            }
            simple_renderer_flush(sr);
        }
    }

    render_fill_column(sr, atlas, editor);

    // Render cursor
    if (isWave) {
        simple_renderer_set_shader(sr, VERTEX_SHADER_WAVE, SHADER_FOR_COLOR);
    } else if (editor->selection) {
        simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_CURSOR);
    } else {
        simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR);
    }

    {
        if (showLineNumbers) {
            cursor_pos.x += lineNumberWidth;
        }

        // Constants and Default Settings
        float CURSOR_WIDTH;
        const Uint32 CURSOR_BLINK_THRESHOLD = 500;
        const Uint32 CURSOR_BLINK_PERIOD = 1000;
        const Uint32 t = SDL_GetTicks() - editor->last_stroke;
        Vec4f CURSOR_COLOR = CURRENT_THEME.cursor;
        float BORDER_THICKNESS = 3.0f;
        Vec4f INNER_COLOR = vec4f(CURSOR_COLOR.x, CURSOR_COLOR.y, CURSOR_COLOR.z, 0.3);

        sr->verticies_count = 0;

        // If editor has a mark, make the cursor transparent
        if (editor->has_mark) {
            /* CURSOR_COLOR.w = 0.0f; // Set alpha to 0 (fully transparent) */
        }

        // Rendering based on mode
        switch (current_mode) {

        case NORMAL: {
            float cursor_width;
            // Check if the cursor is on an actual character or an empty line
            if (editor->cursor < editor->data.count && editor->data.items[editor->cursor] != '\n') {
                    Vec2f next_char_pos = cursor_pos;
                    free_glyph_atlas_measure_line_sized(
                        atlas, editor->data.items + editor->cursor,
                        1, // Measure the actual character at the cursor
                        &next_char_pos);
                    cursor_width = next_char_pos.x - cursor_pos.x;
            } else {
                    cursor_width = whitespace_width;
            }

            simple_renderer_solid_rect(
                sr, cursor_pos, vec2f(cursor_width, FREE_GLYPH_FONT_SIZE),
                CURSOR_COLOR);
        } break;

        case HELIX: {
            float cursor_width;
            // Check if the cursor is on an actual character or an empty line
            if (editor->cursor < editor->data.count && editor->data.items[editor->cursor] != '\n') {
                    Vec2f next_char_pos = cursor_pos;
                    free_glyph_atlas_measure_line_sized(
                        atlas, editor->data.items + editor->cursor,
                        1, // Measure the actual character at the cursor
                        &next_char_pos);
                    cursor_width = next_char_pos.x - cursor_pos.x;
            } else {
                    cursor_width = whitespace_width;
            }

            simple_renderer_solid_rect(
                sr, cursor_pos, vec2f(cursor_width, FREE_GLYPH_FONT_SIZE),
                CURSOR_COLOR);
        } break;

        case VISUAL_LINE:
        case VISUAL: {
            float cursor_width;
            // Check if the cursor is on an actual character or an empty line
            if (editor->cursor < editor->data.count && editor->data.items[editor->cursor] != '\n') {
                Vec2f next_char_pos = cursor_pos;
                free_glyph_atlas_measure_line_sized(atlas, editor->data.items + editor->cursor, 1, &next_char_pos);
                cursor_width = next_char_pos.x - cursor_pos.x;
            } else {
                cursor_width = whitespace_width;
            }

            simple_renderer_solid_rect(sr, cursor_pos, vec2f(cursor_width, FREE_GLYPH_FONT_SIZE), CURSOR_COLOR);
        } break;

        case MINIBUFFER: {
            // TODO
        } break;

        case EMACS: {
            float cursor_width;
            CURSOR_COLOR = CURRENT_THEME.emacs_cursor;
            // Check if the cursor is on an actual character or an empty line
            if (editor->cursor < editor->data.count &&
                editor->data.items[editor->cursor] != '\n') {
                Vec2f next_char_pos = cursor_pos; free_glyph_atlas_measure_line_sized(atlas, editor->data.items + editor->cursor, 1, &next_char_pos);
                cursor_width = next_char_pos.x - cursor_pos.x;
            } else {
                cursor_width = whitespace_width;
            }
            
            // Implement blinking for EMACS mode
            if (t < CURSOR_BLINK_THRESHOLD || (t / CURSOR_BLINK_PERIOD) % 2 != 0) {
                simple_renderer_solid_rect(sr, cursor_pos, vec2f(cursor_width, FREE_GLYPH_FONT_SIZE), CURSOR_COLOR);
            }
        } break;

            
        case INSERT:
            CURSOR_COLOR = themes[currentThemeIndex].insert_cursor;
            /* CURSOR_COLOR = CURRENT_THEME.cursor; */
            if (BlockInsertCursor) {
                // Check if the cursor is on an actual character or an empty line
                if (editor->cursor < editor->data.count && editor->data.items[editor->cursor] != '\n') {
                    Vec2f next_char_pos = cursor_pos;
                    free_glyph_atlas_measure_line_sized(
                                                        atlas, editor->data.items + editor->cursor,
                                                        1, // Measure the actual character at the cursor
                                                        &next_char_pos);
                    CURSOR_WIDTH = next_char_pos.x - cursor_pos.x;
                } else {
                    CURSOR_WIDTH = whitespace_width;
                }
            } else {
                CURSOR_WIDTH = 5.0f; // Thin line
            }
            // blinking for INSERT mode
            if (t < CURSOR_BLINK_THRESHOLD ||
                (t / CURSOR_BLINK_PERIOD) % 2 != 0) {
                    simple_renderer_solid_rect(
                        sr, cursor_pos,
                        vec2f(CURSOR_WIDTH, FREE_GLYPH_FONT_SIZE),
                        CURSOR_COLOR);
            }
            break;

        /* case VISUAL: { */
        /*     float cursor_width; */

        /*     // Check if the cursor is on an actual character or an empty line */
        /*     if (editor->cursor < editor->data.count && */
        /*         editor->data.items[editor->cursor] != '\n') { */
        /*             Vec2f next_char_pos = cursor_pos; */
        /*             free_glyph_atlas_measure_line_sized( */
        /*                 atlas, editor->data.items + editor->cursor, 1, */
        /*                 &next_char_pos); */
        /*             cursor_width = next_char_pos.x - cursor_pos.x; */
        /*     } else { */
        /*             Vec2f next_char_pos = cursor_pos; */
        /*             free_glyph_atlas_measure_line_sized(atlas, "a", 1, */
        /*                                                 &next_char_pos); */
        /*             cursor_width = next_char_pos.x - cursor_pos.x; */
        /*     } */

        /*     // Draw inner rectangle */
        /*     simple_renderer_solid_rect( */
        /*         sr, */
        /*         vec2f(cursor_pos.x + BORDER_THICKNESS, */
        /*               cursor_pos.y + BORDER_THICKNESS), */
        /*         vec2f(cursor_width - 2 * BORDER_THICKNESS, */
        /*               FREE_GLYPH_FONT_SIZE - 2 * BORDER_THICKNESS), */
        /*         INNER_COLOR); */

        /*     // Draw the outline (borders) using the theme's cursor color */
        /*     simple_renderer_solid_rect(sr, cursor_pos, */
        /*                                vec2f(cursor_width, BORDER_THICKNESS), */
        /*                                CURSOR_COLOR); // Top border */
        /*     simple_renderer_solid_rect( */
        /*         sr, */
        /*         vec2f(cursor_pos.x, */
        /*               cursor_pos.y + FREE_GLYPH_FONT_SIZE - BORDER_THICKNESS), */
        /*         vec2f(cursor_width, BORDER_THICKNESS), */
        /*         CURSOR_COLOR); // Bottom border */
        /*     simple_renderer_solid_rect( */
        /*         sr, cursor_pos, vec2f(BORDER_THICKNESS, FREE_GLYPH_FONT_SIZE), */
        /*         CURSOR_COLOR); // Left border */
        /*     simple_renderer_solid_rect( */
        /*         sr, */
        /*         vec2f(cursor_pos.x + cursor_width - BORDER_THICKNESS, */
        /*               cursor_pos.y), */
        /*         vec2f(BORDER_THICKNESS, FREE_GLYPH_FONT_SIZE), */
        /*         CURSOR_COLOR); // Right border */

        /*     break; */
        /* } */

        }
        simple_renderer_flush(sr);
    }

    
    // Render marked search result
    {
        simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR);
        if (editor->has_mark) {
            for (size_t row = 0; row < editor->lines.count; ++row) {
                size_t mark_begin_chr = editor->mark_start;
                size_t mark_end_chr = editor->mark_end;

                Line line_chr = editor->lines.items[row];

                if (mark_begin_chr < line_chr.begin) {
                    mark_begin_chr = line_chr.begin;
                }

                if (mark_end_chr > line_chr.end) {
                    mark_end_chr = line_chr.end;
                }

                if (mark_begin_chr <= mark_end_chr) {
                    Vec2f mark_begin_scr = vec2f(0, -((float)row + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE);
                    free_glyph_atlas_measure_line_sized(
                                                        atlas, editor->data.items + line_chr.begin, mark_begin_chr - line_chr.begin,
                                                        &mark_begin_scr);

                    Vec2f mark_end_scr = mark_begin_scr;
                    free_glyph_atlas_measure_line_sized(
                                                        atlas, editor->data.items + mark_begin_chr, mark_end_chr - mark_begin_chr,
                                                        &mark_end_scr);

                    // Adjust for line numbers width if they are displayed
                    if (showLineNumbers) {
                        mark_begin_scr.x += lineNumberWidth;
                        mark_end_scr.x += lineNumberWidth;
                    }

                    Vec4f mark_color = CURRENT_THEME.marks;
                    simple_renderer_solid_rect(sr, mark_begin_scr, vec2f(mark_end_scr.x - mark_begin_scr.x, FREE_GLYPH_FONT_SIZE), mark_color);
                }
            }
        }
        simple_renderer_flush(sr);
    }


    
    // Render text
    {

        if (isWave) {
            simple_renderer_set_shader(sr, VERTEX_SHADER_WAVE, SHADER_FOR_TEXT);
        } else {
            simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_TEXT);
        }

        for (size_t i = 0; i < editor->tokens.count; ++i) {
            Token token = editor->tokens.items[i];
            Vec2f pos = token.position;
            //Vec4f color = vec4fs(1);
            // TODO match color for open and close
            Vec4f color = CURRENT_THEME.text;
            
            // Adjust for line numbers width if they are displayed
            if (showLineNumbers) {
                pos.x += lineNumberWidth;
            }
            
            switch (token.kind) {
            case TOKEN_PREPROC:
                if (token.text_len >= 7 && token.text[0] == '#') { // Check if it's likely a hex color
                    bool valid_hex = true;
                    for (size_t j = 1; j < 7 && valid_hex; ++j) {
                        if (!is_hex_digit(token.text[j])) {
                            valid_hex = false;
                        }
                    }

                    if (valid_hex) {
                        unsigned int hex_value;
                        if(sscanf(token.text, "#%06x", &hex_value) == 1) {
                            color = hex_to_vec4f(hex_value);
                        } else {
                            color = CURRENT_THEME.hashtag; // Default to the hashtag color if not a valid hex
                        }
                    } else {
                        color = CURRENT_THEME.hashtag; // Not a valid hex color
                    }
                } else {
                    color = CURRENT_THEME.hashtag; // Default color for preprocessor directives
                }
                break;

            case TOKEN_KEYWORD:
                color = CURRENT_THEME.logic;
                break;
                
            case TOKEN_TYPE:
                color = CURRENT_THEME.type;
                break;

            case TOKEN_NULL:
                color = CURRENT_THEME.null;
                break;
                
            case TOKEN_FUNCTION_DEFINITION:
                color = CURRENT_THEME.function_definition;
                break;

            case TOKEN_LINK:
                color = CURRENT_THEME.link;
                break;

            case TOKEN_OR:
                color = CURRENT_THEME.logic_or;
                break;

            case TOKEN_PIPE:
                color = CURRENT_THEME.pipe;
                break;

            case TOKEN_AND:
                color = CURRENT_THEME.logic_and;
                break;

            case TOKEN_AMPERSAND:
                color = CURRENT_THEME.ampersand;
                break;

            case TOKEN_POINTER:
                color = CURRENT_THEME.pointer;
                break;

            case TOKEN_MULTIPLICATION:
                color = CURRENT_THEME.multiplication;
                break;

            case TOKEN_COMMENT:
                {
                    color = CURRENT_THEME.comment;

                    // Checking for TODOOOO...
                    char* todoLoc = strstr(token.text, "TODO");
                    if (todoLoc && (size_t)(todoLoc - token.text + 3) < token.text_len) {

                        size_t numOs = 0;
                        char* ptr = todoLoc + 4; // Start right after "TODO"

                        // Count 'O's without crossing token boundary
                        while ((size_t)(ptr - token.text) < token.text_len && (*ptr == 'O' || *ptr == 'o')) {

                            numOs++;
                            ptr++;
                        }

                        Vec4f baseColor = CURRENT_THEME.todo;
                        float deltaRed = (1.0f - baseColor.x) / 5;  // Adjusting for maximum of TODOOOOO

                        color.x = baseColor.x + deltaRed * numOs;
                        color.y = baseColor.y * (1 - 0.2 * numOs);
                        color.z = baseColor.z * (1 - 0.2 * numOs);
                        color.w = baseColor.w;
                    }

                    // Checking for FIXMEEEE...
                    char* fixmeLoc = strstr(token.text, "FIXME");
                    if (fixmeLoc && (size_t)(fixmeLoc - token.text + 4) < token.text_len) {

                        size_t numEs = 0;
                        char* ptr = fixmeLoc + 5; // Start right after "FIXME"

                        // Count 'E's without crossing token boundary
                        while ((size_t)(ptr - token.text) < token.text_len && (*ptr == 'E' || *ptr == 'e')) {

                            numEs++;
                            ptr++;
                        }

                        Vec4f baseColor = CURRENT_THEME.fixme;
                        float deltaRed = (1.0f - baseColor.x) / 5;  // Adjusting for maximum of FIXMEEEE

                        color.x = baseColor.x + deltaRed * numEs;
                        color.y = baseColor.y * (1 - 0.2 * numEs);
                        color.z = baseColor.z * (1 - 0.2 * numEs);
                        color.w = baseColor.w;
                    }

                    // Checking for BUG...
                    char* bugLoc = strstr(token.text, "BUG");
                    if (bugLoc && (size_t)(bugLoc - token.text + 2) < token.text_len) {

                        color = CURRENT_THEME.bug;
                    }


                    // Checking for NOTE...
                    char* noteLoc = strstr(token.text, "NOTE");
                    if (noteLoc && (size_t)(noteLoc - token.text + 3) < token.text_len) {

                        color = CURRENT_THEME.note;
                    }

                    // Continue rendering with
                }
                break;


            case TOKEN_EQUALS:
                color = CURRENT_THEME.equals;
                break;

            case TOKEN_EXCLAMATION:
                color = CURRENT_THEME.exclamation;
                break;

            case TOKEN_NOT_EQUALS:
                color = CURRENT_THEME.not_equals;
                break;

            case TOKEN_EQUALS_EQUALS:
                color = CURRENT_THEME.equals_equals;
                break;


            case TOKEN_LESS_THAN:
                color = CURRENT_THEME.less_than;
                break;

            case TOKEN_GREATER_THAN:
                color = CURRENT_THEME.greater_than;
                break;
            case TOKEN_ARROW:
                color = CURRENT_THEME.arrow;
                break;

            case TOKEN_MINUS:
                color = CURRENT_THEME.minus;
                break;

            case TOKEN_PLUS:
                color = CURRENT_THEME.plus;
                break;

            case TOKEN_TRUE:
                color = CURRENT_THEME.truee;
                break;
            case TOKEN_FALSE:
                color = CURRENT_THEME.falsee;
                break;
            case TOKEN_OPEN_SQUARE:
                color = CURRENT_THEME.open_square;
                break;
            case TOKEN_CLOSE_SQUARE:
                color = CURRENT_THEME.close_square;
                break;
            case TOKEN_ARRAY_CONTENT:
                color = CURRENT_THEME.array_content;
                break;
            case TOKEN_BAD_SPELLCHECK:
                color = CURRENT_THEME.bug;
                break;
            case TOKEN_STRING:
                color = CURRENT_THEME.string;
                break;
            case TOKEN_OPEN_CURLY:
                color = CURRENT_THEME.string;
                break;
            case TOKEN_CLOSE_CURLY:
                color = CURRENT_THEME.string;
                break;

            case TOKEN_COLOR: {
                    unsigned long long hex_value;
                    if(sscanf(token.text, "0x%llx", &hex_value) == 1) {
                        color = hex_to_vec4f((uint32_t)hex_value);
                    }
                }
                break;
            default:
                {}
            }
            
            free_glyph_atlas_render_line_sized(atlas, sr, token.text, token.text_len, &pos, color);
            // TODO: the max_line_len should be calculated based on what's visible on the screen right now
            if (max_line_len < pos.x) max_line_len = pos.x;
        }
        simple_renderer_flush(sr);
    }

    
    
   
    // Render minibuffer
    {
        if (showMinibuffer) {
            simple_renderer_set_shader(sr, VERTEX_SHADER_FIXED, SHADER_FOR_COLOR);
            simple_renderer_solid_rect(sr, (Vec2f){0.0f, 0.0f}, (Vec2f){w, minibufferHeight}, CURRENT_THEME.minibuffer);
            simple_renderer_flush(sr);
        }
    }


    // Render modeline
    {
        if (showModeline) {
            simple_renderer_set_shader(sr, VERTEX_SHADER_FIXED, SHADER_FOR_COLOR);
            simple_renderer_solid_rect(sr, (Vec2f){0.0f, minibufferHeight}, (Vec2f){w, modelineHeight}, CURRENT_THEME.modeline);
            // render accent
            simple_renderer_solid_rect(sr, (Vec2f){0.0f, minibufferHeight}, (Vec2f){modelineAccentWidth, modelineHeight}, CURRENT_THEME.modeline_accent);
            simple_renderer_flush(sr);
        }
    }


    // Render fringe
    {
        simple_renderer_set_shader(sr, VERTEX_SHADER_FIXED, SHADER_FOR_COLOR);
        simple_renderer_solid_rect(sr, (Vec2f){0.0f, modelineHeight + minibufferHeight}, (Vec2f){fringeWidth, h}, CURRENT_THEME.fringe);
        simple_renderer_flush(sr);
    }

    // render clock
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int hours = tm->tm_hour;
    int minutes = tm->tm_min;
    render_clock(sr, hours, minutes);


    // Update camera
    {
        if (followCursor && !instantCamera) {
            if (automatic_zoom) {
                float len = calculate_max_line_length(editor);
                if (len > 0) { // Check if there is at least one line
                    if (len <= 62) {
                        zoom_factor = 4.0f;
                    } else if (len <= 78) {
                        zoom_factor = 5.0f;
                    } else if (len <= 94) {
                        zoom_factor = 6.0f;
                    } else {
                        zoom_factor = 5.0f;
                    }

                    if (showLineNumbers) {
                        zoom_factor += 1.5f;
                    }                        
                    if (zoomInInsertMode) {
                        if (current_mode == INSERT) {
                            zoom_factor -= 1;
                        }
                    }
                }
            }
            
            if (max_line_len > 1000.0f) {
                max_line_len = 1000.0f;
            }

            float target_scale = w / zoom_factor / (max_line_len * 0.75); // TODO: division by 0

            Vec2f target = cursor_pos;
            float offset = 0.0f;

            if (target_scale > 3.0f) {
                target_scale = 3.0f;
            } else {

                float division;
                if (centeredText) {
                    division = 3;
                } else {
                    division = 2.02;
                }

                offset = cursor_pos.x - w/3/sr->camera_scale;
                if (offset < 0.0f) offset = 0.0f;
                target = vec2f(w/division/sr->camera_scale + offset, cursor_pos.y);
            }

            sr->camera_vel = vec2f_mul(
                                       vec2f_sub(target, sr->camera_pos),
                                       vec2fs(2.0f));
            sr->camera_scale_vel = (target_scale - sr->camera_scale) * 2.0f;

            sr->camera_pos = vec2f_add(sr->camera_pos, vec2f_mul(sr->camera_vel, vec2fs(DELTA_TIME)));
            sr->camera_scale = sr->camera_scale + sr->camera_scale_vel * DELTA_TIME;

        } else if (followCursor && instantCamera) {
            // TODO looks bas maybe implement double buffering
            if (max_line_len > 1000.0f) {
                max_line_len = 1000.0f;
            }
            
            float target_scale = w / zoom_factor / (max_line_len * 0.75); // Handle potential division by 0
            
            Vec2f target = cursor_pos;
            float offset = 0.0f;

            float division;
            if (centeredText) {
                division = 3;
            } else {
                division = 2.02;
            }
            
            if (target_scale > 3.0f) {
                target_scale = 3.0f;
            } else {
                offset = cursor_pos.x - w/3/sr->camera_scale;
                if (offset < 0.0f) offset = 0.0f;
                target = vec2f(w/division/sr->camera_scale + offset, cursor_pos.y);
            }
            
            // Instantly set the camera position and scale
            sr->camera_pos = target;
            sr->camera_scale = target_scale;
            
        } else {
            sr->camera_scale = 0.33f;  // 0.24

            // Static flag to ensure initial camera position is set only once
            static bool hasSetInitialPosition = false;

            // If the initial position hasn't been set, set it now
            if (!hasSetInitialPosition) {
                /* sr->camera_pos.x = 2870.0f;  // Set the x-position */

                if (showLineNumbers) {
                    sr->camera_pos.x = 2855.0f;
                } else {
                    sr->camera_pos.x = 2890.0f;
                }
                sr->camera_pos.y = -4000.0f;  // Set the initial y-position
                hasSetInitialPosition = true;
            } else {
                // Calculate the vertical position of the cursor in world coordinates.
                int currentLine = editor_cursor_row(editor);
                float cursorPosY = -((float)currentLine + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE;

                // Define the top and bottom edges of the current camera view.
                float cameraTopEdge = sr->camera_pos.y - (h * 1/2.3f) / sr->camera_scale;
                float cameraBottomEdge = sr->camera_pos.y + (h * 1/2.3f) / sr->camera_scale;

        
                // Adjust the camera's Y position if the cursor is outside the viewport.
                if (cursorPosY > cameraBottomEdge) {
                    sr->camera_pos.y += cursorPosY - cameraBottomEdge;  // Move camera down just enough
                } else if (cursorPosY < cameraTopEdge) {
                    sr->camera_pos.y -= cameraTopEdge - cursorPosY;  // Move camera up just enough
                }

                // Keeping the x-position fixed as per the previous logic
                if (showLineNumbers) {
                    sr->camera_pos.x = 2855.0f;
                } else {
                    sr->camera_pos.x = 2890.0f;
                }
            }
        }
    }
}



