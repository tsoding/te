#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "./editor.h"
#include "./common.h"

void editor_backspace(Editor *e)
{
    if (e->input.active) {
        if (e->input.text.count > 0) {
            e->input.text.count -= 1;
        }
    } else {
        if (e->cursor > e->data.count) {
            e->cursor = e->data.count;
        }
        if (e->cursor == 0) return;

        memmove(
            &e->data.items[e->cursor - 1],
            &e->data.items[e->cursor],
            e->data.count - e->cursor
        );
        e->cursor -= 1;
        e->data.count -= 1;
        editor_retokenize(e);
    }
}

void editor_delete(Editor *e)
{
    if (e->searching) return;

    if (e->cursor >= e->data.count) return;
    memmove(
        &e->data.items[e->cursor],
        &e->data.items[e->cursor + 1],
        e->data.count - e->cursor - 1
    );
    e->data.count -= 1;
    editor_retokenize(e);
}

void editor_delete_selection(Editor *e)
{
    assert(e->selection);

    if (e->cursor > e->select_begin) {
        if (e->cursor > e->data.count) {
            e->cursor = e->data.count;
        }
        if (e->cursor == 0) return;

        size_t nchars = e->cursor - e->select_begin;
        memmove(
            &e->data.items[e->cursor - nchars],
            &e->data.items[e->cursor],
            e->data.count - e->cursor
        );

        e->cursor -= nchars;
        e->data.count -= nchars;
    } else {
        if (e->cursor >= e->data.count) return;

        size_t nchars = e->select_begin - e->cursor;
        memmove(
            &e->data.items[e->cursor],
            &e->data.items[e->cursor + nchars],
            e->data.count - e->cursor - nchars
        );

        e->data.count -= nchars;
    }
    editor_retokenize(e);
}

Errno editor_save_as(Editor *e, const char *file_path)
{
    printf("Saving as %s...\n", file_path);
    Errno err = write_entire_file(file_path, e->data.items, e->data.count);
    if (err != 0) return err;
    e->file_path.count = 0;
    sb_append_cstr(&e->file_path, file_path);
    sb_append_null(&e->file_path);

    editor_configured_popup(e, "save ok", (PlaceholderList) {
        .elems = (Placeholder[]) {
            PLACEHOLDER_STR("path", file_path),
        },
        .elems_size = 1
    });

    return 0;
}

Errno editor_save(Editor *e)
{
    assert(e->file_path.count > 0);
    printf("Saving as %s...\n", e->file_path.items);
    Errno err = write_entire_file(e->file_path.items, e->data.items, e->data.count);
    if (err == 0) {
        editor_configured_popup(e, "save ok", (PlaceholderList) {
            .elems = (Placeholder[]) {
                PLACEHOLDER_STR("path", e->file_path.items),
            },
            .elems_size = 1
        });
    }
    return err;
}

Errno editor_load_from_file(Editor *e, const char *file_path)
{
    printf("Loading %s\n", file_path);

    e->data.count = 0;
    Errno err = read_entire_file(file_path, &e->data);
    if (err != 0) return err;

    e->cursor = 0;

    editor_retokenize(e);

    e->file_path.count = 0;
    sb_append_cstr(&e->file_path, file_path);
    sb_append_null(&e->file_path);

    return 0;
}

size_t editor_cursor_row(const Editor *e)
{
    assert(e->lines.count > 0);
    for (size_t row = 0; row < e->lines.count; ++row) {
        Line line = e->lines.items[row];
        if (line.begin <= e->cursor && e->cursor <= line.end) {
            return row;
        }
    }
    return e->lines.count - 1;
}

void editor_goto(Editor *e, size_t line, size_t col)
{
    if (e->lines.count == 0)
        line = 0;
    else if (line >= e->lines.count)
        line = e->lines.count - 1;

    Line target_line = e->lines.items[line];
    size_t target_line_size = target_line.end - target_line.begin;
    if (target_line_size == 0)
        col = 0;
    else if (col >= target_line_size)
        col = target_line_size - 1;
    e->cursor = target_line.begin + col;
}

void editor_move_line_up(Editor *e)
{
    editor_stop_search(e);

    size_t cursor_row = editor_cursor_row(e);
    size_t cursor_col = e->cursor - e->lines.items[cursor_row].begin;
    if (cursor_row > 0) {
        Line next_line = e->lines.items[cursor_row - 1];
        size_t next_line_size = next_line.end - next_line.begin;
        if (cursor_col > next_line_size) cursor_col = next_line_size;
        e->cursor = next_line.begin + cursor_col;
    }
}

void editor_move_line_down(Editor *e)
{
    editor_stop_search(e);

    size_t cursor_row = editor_cursor_row(e);
    size_t cursor_col = e->cursor - e->lines.items[cursor_row].begin;
    if (cursor_row < e->lines.count - 1) {
        Line next_line = e->lines.items[cursor_row + 1];
        size_t next_line_size = next_line.end - next_line.begin;
        if (cursor_col > next_line_size) cursor_col = next_line_size;
        e->cursor = next_line.begin + cursor_col;
    }
}

void editor_move_char_left(Editor *e)
{
    editor_stop_search(e);
    if (e->cursor > 0) e->cursor -= 1;
}

void editor_move_char_right(Editor *e)
{
    editor_stop_search(e);
    if (e->cursor < e->data.count) e->cursor += 1;
}

void editor_move_word_left(Editor *e)
{
    editor_stop_search(e);
    while (e->cursor > 0 && !isalnum(e->data.items[e->cursor - 1])) {
        e->cursor -= 1;
    }
    while (e->cursor > 0 && isalnum(e->data.items[e->cursor - 1])) {
        e->cursor -= 1;
    }
}

void editor_move_word_right(Editor *e)
{
    editor_stop_search(e);
    while (e->cursor < e->data.count && !isalnum(e->data.items[e->cursor])) {
        e->cursor += 1;
    }
    while (e->cursor < e->data.count && isalnum(e->data.items[e->cursor])) {
        e->cursor += 1;
    }
}

void editor_insert_char(Editor *e, char x)
{
    editor_insert_buf(e, &x, 1);
}

void editor_insert_buf(Editor *e, char *buf, size_t buf_len)
{
    if (e->cursor > e->data.count) {
        e->cursor = e->data.count;
    }

    for (size_t i = 0; i < buf_len; ++i) {
        da_append(&e->data, '\0');
    }
    memmove(
        &e->data.items[e->cursor + buf_len],
        &e->data.items[e->cursor],
        e->data.count - e->cursor - buf_len
    );
    memcpy(&e->data.items[e->cursor], buf, buf_len);
    e->cursor += buf_len;
    editor_retokenize(e);
}

void editor_retokenize(Editor *e)
{
    // Lines
    {
        e->lines.count = 0;

        Line line;
        line.begin = 0;

        for (size_t i = 0; i < e->data.count; ++i) {
            if (e->data.items[i] == '\n') {
                line.end = i;
                da_append(&e->lines, line);
                line.begin = i + 1;
            }
        }

        line.end = e->data.count;
        da_append(&e->lines, line);
    }

    // Syntax Highlighting
    {
        e->tokens.count = 0;
        Lexer l = lexer_new(e->atlas, e->data.items, e->data.count, e->file_path);
        e->file_ext = l.file_ext;
        Token t = lexer_next(&l);
        while (t.kind != TOKEN_END) {
            da_append(&e->tokens, t);
            t = lexer_next(&l);
        }
    }
}

bool editor_line_starts_with(Editor *e, size_t row, size_t col, const char *prefix)
{
    size_t prefix_len = strlen(prefix);
    if (prefix_len == 0) {
        return true;
    }
    Line line = e->lines.items[row];
    if (col + prefix_len - 1 >= line.end) {
        return false;
    }
    for (size_t i = 0; i < prefix_len; ++i) {
        if (prefix[i] != e->data.items[line.begin + col + i]) {
            return false;
        }
    }
    return true;
}

const char *editor_line_starts_with_one_of(Editor *e, size_t row, size_t col, const char **prefixes, size_t prefixes_count)
{
    for (size_t i = 0; i < prefixes_count; ++i) {
        if (editor_line_starts_with(e, row, col, prefixes[i])) {
            return prefixes[i];
        }
    }
    return NULL;
}

void editor_render(SDL_Window *window, Free_Glyph_Atlas *atlas, Simple_Renderer *sr, Editor *editor)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    float max_line_len = 0.0f;

    sr->resolution = vec2f(w, h);
    sr->time = (float) SDL_GetTicks() / 1000.0f;

    // Render selection
    {
        simple_renderer_set_shader(sr, SHADER_FOR_COLOR);
        if (editor->selection) {
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
                    free_glyph_atlas_measure_line_sized(
                        atlas, editor->data.items + line_chr.begin, select_begin_chr - line_chr.begin,
                        &select_begin_scr);

                    Vec2f select_end_scr = select_begin_scr;
                    free_glyph_atlas_measure_line_sized(
                        atlas, editor->data.items + select_begin_chr, select_end_chr - select_begin_chr,
                        &select_end_scr);

                    Vec4f selection_color = vec4f(.25, .25, .25, 1);
                    simple_renderer_solid_rect(sr, select_begin_scr, vec2f(select_end_scr.x - select_begin_scr.x, FREE_GLYPH_FONT_SIZE), selection_color);
                }
            }
        }
        simple_renderer_flush(sr);
    }

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
        if (editor->searching && editor_search_matches_at(editor, editor->cursor)) {
            simple_renderer_set_shader(sr, SHADER_FOR_COLOR);
            Vec4f selection_color = vec4f(.10, .10, .25, 1);
            Vec2f p1 = cursor_pos;
            Vec2f p2 = p1;
            free_glyph_atlas_measure_line_sized(editor->atlas, editor->input.text.items, editor->input.text.count, &p2);
            simple_renderer_solid_rect(sr, p1, vec2f(p2.x - p1.x, FREE_GLYPH_FONT_SIZE), selection_color);
            simple_renderer_flush(sr);
        }
    }

    // Render text
    {
        simple_renderer_set_shader(sr, SHADER_FOR_TEXT);
        for (size_t i = 0; i < editor->tokens.count; ++i) {
            Token token = editor->tokens.items[i];
            Vec2f pos = token.position;
            Vec4f color = vec4fs(1);
            switch (token.kind) {
            case TOKEN_PREPROC:
                color = hex_to_vec4f(0x95A99FFF);
                break;
            case TOKEN_KEYWORD:
                color = hex_to_vec4f(0xFFDD33FF);
                break;
            case TOKEN_COMMENT:
                color = hex_to_vec4f(0xCC8C3CFF);
                break;
            case TOKEN_STRING:
                color = hex_to_vec4f(0x73c936ff);
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

    // Render cursor
    simple_renderer_set_shader(sr, SHADER_FOR_COLOR);
    {
        float CURSOR_WIDTH = 5.0f;
        Uint32 CURSOR_BLINK_THRESHOLD = 500;
        Uint32 CURSOR_BLINK_PERIOD = 1000;
        Uint32 t = SDL_GetTicks() - editor->last_stroke;

        sr->verticies_count = 0;
        if (t < CURSOR_BLINK_THRESHOLD || t/CURSOR_BLINK_PERIOD%2 != 0) {
            simple_renderer_solid_rect(
                sr,
                cursor_pos, vec2f(CURSOR_WIDTH, FREE_GLYPH_FONT_SIZE),
                vec4fs(1));
        }

        simple_renderer_flush(sr);
    }

    // Render pop-ups
    {
        float scale = (float) editor->configs.popup.scale;
        float oscale = 1.0f / scale;

        Simple_Camera oldCam = sr->cam;
        sr->cam = (Simple_Camera) {
            .pos = vec2f((float) w / 2 * oscale - 20.0f * scale, -((float) h / 2 * oscale) + 80.0f * scale),
            .scale = scale,
            .scale_vel = 0.0f,
            .vel = vec2f(0, 0)
        };
        simple_renderer_set_shader(sr, SHADER_FOR_TEXT);

        for (size_t i = 0; i < editor->popUps.count; i ++) {
            PopUp *p = &editor->popUps.items[i];
            if (p->lasts == 0)
                continue;

            if (p->when + p->lasts < SDL_GetTicks()) {
                editor_remove_popup(editor, p->uid);
                i --;
            }
        }

        for (size_t i = 0; i < editor->popUps.count; i ++) {
            PopUp *p = &editor->popUps.items[i];

            float t = (SDL_GetTicks() - p->when) / (float) editor->configs.popup.fade_in;
            if (t > 1)
                t = 1;
            else if (t < 0)
                t = 0;

            Vec2f pos = vec2f(
                lerpf(-180 * oscale, 0, t),
                -(i * 80.0f * scale)
            );
            free_glyph_atlas_render_line_sized(atlas, sr, p->msg, p->msg_size, &pos, p->color);
        }

        simple_renderer_flush(sr);
        sr->cam = oldCam;
    }

    // Render bottom bar
    {
        float scale = 0.6f;
        float oscale = 1.0f / scale;

        Simple_Camera oldCam = sr->cam;
        sr->cam = (Simple_Camera) {
            .pos = vec2f((float) w / 2 * oscale, ((float) h / 2 * oscale) - 60.0f * scale),
            .scale = scale,
            .scale_vel = 0.0f,
            .vel = vec2f(0, 0)
        };

        {
            simple_renderer_set_shader(sr, SHADER_FOR_COLOR);
            Vec4f bg = hex_to_vec4f(0xe7e7e7ff);
            Vec2f p1 = vec2f(0, -60.0f * scale);
            Vec2f s = vec2f(w * oscale, 60.0f + 60.0f * scale);
            simple_renderer_solid_rect(sr, p1, s, bg);
            simple_renderer_flush(sr);
        }

        float x = 20.0f * scale;

        // Render input
        if (editor->input.active) {
            {
                simple_renderer_set_shader(sr, SHADER_FOR_TEXT);
                Vec4f color = hex_to_vec4f(0x5b5b5bFF);
                Vec2f pos = vec2f(x, -20 * scale);
                free_glyph_atlas_render_line_sized(atlas, sr,
                                                   editor->input.hint,
                                                   editor->input.hint_len,
                                                   &pos, color);
                x = pos.x;
                simple_renderer_flush(sr);
            }

            {
                simple_renderer_set_shader(sr, SHADER_FOR_TEXT);
                Vec4f color = hex_to_vec4f(0x181818FF);
                Vec2f pos = vec2f(x, -20 * scale);
                free_glyph_atlas_render_line_sized(atlas, sr,
                                                   editor->input.text.items,
                                                   editor->input.text.count,
                                                   &pos, color);
                simple_renderer_flush(sr);
            }
        }
        // Render additional info
        else {
            static char str[200];
            sprintf(str, "%s  %zu / %zu", file_ext_str(editor->file_ext), editor_cursor_row(editor) + 1, editor->lines.count);

            {
                simple_renderer_set_shader(sr, SHADER_FOR_TEXT);
                Vec4f color = hex_to_vec4f(0x181818FF);
                Vec2f pos = vec2f(x, -20 * scale);
                free_glyph_atlas_render_line_sized(atlas, sr,
                                                   str,
                                                   strlen(str),
                                                   &pos, color);
                x = pos.x;
                simple_renderer_flush(sr);
            }
        }

        sr->cam = oldCam;
    }

    // Update camera
    {
        if (max_line_len > 1000.0f) {
            max_line_len = 1000.0f;
        }

        float target_scale = w/3/(max_line_len*0.75); // TODO: division by 0

        Vec2f target = cursor_pos;
        float offset = 0.0f;

        if (target_scale > 3.0f) {
            target_scale = 3.0f;
        } else {
            offset = cursor_pos.x - w/3/sr->cam.scale;
            if (offset < 0.0f) offset = 0.0f;
            target = vec2f(w/3/sr->cam.scale + offset, cursor_pos.y);
        }

        sr->cam.vel = vec2f_mul(
                             vec2f_sub(target, sr->cam.pos),
                             vec2fs(2.0f));
        sr->cam.scale_vel = (target_scale - sr->cam.scale) * 2.0f;

        sr->cam.pos = vec2f_add(sr->cam.pos, vec2f_mul(sr->cam.vel, vec2fs(DELTA_TIME)));
        sr->cam.scale = sr->cam.scale + sr->cam.scale_vel * DELTA_TIME;
    }
}

void editor_update_selection(Editor *e, bool shift)
{
    if (e->searching) return;
    if (shift) {
        if (!e->selection) {
            e->selection = true;
            e->select_begin = e->cursor;
        }
    } else {
        if (e->selection) {
            e->selection = false;
        }
    }
}

void editor_clipboard_copy(Editor *e)
{
    if (e->searching) return;
    if (e->selection) {
        size_t begin = e->select_begin;
        size_t end = e->cursor;
        if (begin > end) SWAP(size_t, begin, end);

        e->clipboard.count = 0;
        sb_append_buf(&e->clipboard, &e->data.items[begin], end - begin + 1);
        sb_append_null(&e->clipboard);

        if (SDL_SetClipboardText(e->clipboard.items) < 0) {
            fprintf(stderr, "ERROR: SDL ERROR: %s\n", SDL_GetError());
        }
    }
}

void editor_clipboard_paste(Editor *e)
{
    char *text = SDL_GetClipboardText();
    size_t text_len = strlen(text);
    if (text_len > 0) {
        editor_insert_buf(e, text, text_len);
    } else {
        fprintf(stderr, "ERROR: SDL ERROR: %s\n", SDL_GetError());
    }
    SDL_free(text);
}

void editor_start_search(Editor *e)
{
    if (e->searching) {
        for (size_t pos = e->cursor + 1; pos < e->data.count; ++pos) {
            if (editor_search_matches_at(e, pos)) {
                e->cursor = pos;
                break;
            }
        }
    } else {
        e->searching = true;
        editor_start_input(e);
        e->input.hint = editor_configured_inline_hint(e, "find");
        e->input.hint_len = strlen(e->input.hint);
        if (e->selection) {
            size_t begin = e->select_begin;
            size_t end = e->cursor;
            if (begin > end) SWAP(size_t, begin, end);

            sb_append_buf(&e->input.text, &e->data.items[begin], end - begin + 1);
            e->cursor = begin;
            e->selection = false;
        }
    }
}

void editor_stop_search(Editor *e)
{
    e->searching = false;
    if (!e->input.required)
        e->input.active = false;
}

bool editor_search_matches_at(Editor *e, size_t pos)
{
    if (e->data.count - pos < e->input.text.count) return false;
    for (size_t i = 0; i < e->input.text.count; ++i) {
        if (e->input.text.items[i] != e->data.items[pos + i]) {
            return false;
        }
    }
    return true;
}

void editor_move_to_begin(Editor *e)
{
    editor_stop_search(e);
    e->cursor = 0;
}

void editor_move_to_end(Editor *e)
{
    editor_stop_search(e);
    e->cursor = e->data.count;
}

void editor_move_to_line_begin(Editor *e)
{
    editor_stop_search(e);
    size_t row = editor_cursor_row(e);
    e->cursor = e->lines.items[row].begin;
}

void editor_move_to_line_end(Editor *e)
{
    editor_stop_search(e);
    size_t row = editor_cursor_row(e);
    e->cursor = e->lines.items[row].end;
}

void editor_move_paragraph_up(Editor *e)
{
    editor_stop_search(e);
    size_t row = editor_cursor_row(e);
    while (row > 0 && e->lines.items[row].end - e->lines.items[row].begin <= 1) {
        row -= 1;
    }
    while (row > 0 && e->lines.items[row].end - e->lines.items[row].begin > 1) {
        row -= 1;
    }
    e->cursor = e->lines.items[row].begin;
}

void editor_move_paragraph_down(Editor *e)
{
    editor_stop_search(e);
    size_t row = editor_cursor_row(e);
    while (row + 1 < e->lines.count && e->lines.items[row].end - e->lines.items[row].begin <= 1) {
        row += 1;
    }
    while (row + 1 < e->lines.count && e->lines.items[row].end - e->lines.items[row].begin > 1) {
        row += 1;
    }
    e->cursor = e->lines.items[row].begin;
}

Uint32 nextPopUpUid = 0;
Uint32 editor_add_popup(Editor *editor, PopUp *popUp)
{
    editor->popUps.count ++;
    editor->popUps.items = realloc(editor->popUps.items, sizeof(PopUp) * editor->popUps.count);
    memcpy(&editor->popUps.items[editor->popUps.count - 1], popUp, sizeof(PopUp));
    editor->popUps.items[editor->popUps.count - 1].uid = nextPopUpUid;
    return nextPopUpUid ++;
}

void editor_remove_popup(Editor *editor, Uint32 uid)
{
    for (size_t i = 0; i < editor->popUps.count; i ++) {
        PopUp *p = &editor->popUps.items[i];
        if (p->uid != uid)
            continue;

        if (p->free_msg)
            free((char *) p->msg);

        memcpy(p, p + 1, (editor->popUps.count - i - 1) * sizeof(PopUp));
        editor->popUps.count --;
        editor->popUps.items = realloc(editor->popUps.items, sizeof(PopUp) * editor->popUps.count);
        break;
    }
}

void flash_error_str(Editor *editor, const char *str)
{
    fputs(str, stderr);
    fputc('\n', stderr);

    PopUp p;
    p.msg = str;
    p.free_msg = false;
    p.msg_size = strlen(str);
    p.color = hex_to_vec4f(0xff2400ff);
    p.when = SDL_GetTicks();
    p.lasts = 2000;

    (void) editor_add_popup(editor, &p);
}

void editor_start_input(Editor *editor)
{
    editor->input.active = true;
    editor->input.onDone = NULL;
    editor->input.required = false;
    editor->input.hint_len = 0;
    if (editor->input.text.items) {
        free(editor->input.text.items);
        editor->input.text = (String_Builder){0};
    }
}

void editor_configured_popup(Editor *editor, const char *type, PlaceholderList placeholders) {
    Config cfg;
    config_init(&cfg);
    bool ok;
    config_child(&cfg, editor->configs.popup_messages, type, &ok);
    if (!ok) {
        fprintf(stderr, "\"pop ups/%s\" not configured!\n", type);
        goto err;
    }

    PopUp popup;

    const char *fmt_str = config_get_str_at(cfg, "str", &ok);
    if (!ok) {
        fprintf(stderr, "\"pop ups/%s/str\" not configured!\n", type);
        goto err;
    }

    popup.color = hex_to_vec4f(config_get_long_at(cfg, "color", &ok));
    if (!ok) {
        fprintf(stderr, "\"pop ups/%s/color\" not configured!\n", type);
        goto err;
    }

    popup.lasts = config_get_long_at(cfg, "last", &ok);
    if (!ok) {
        fprintf(stderr, "\"pop ups/%s/last\" not configured!\n", type);
        goto err;
    }

    {
        size_t fmt_str_len = strlen(fmt_str);
        char *fmt_str_copy = malloc(fmt_str_len + 1);
        memcpy(fmt_str_copy, fmt_str, fmt_str_len + 1);

        Fmt fmt = fmt_compile(fmt_str_copy);
        popup.msg = fmt_fmt_fmt(fmt, placeholders);
        fmt_destroy(fmt);

        free(fmt_str_copy);

        if (popup.msg == NULL) {
            fprintf(stderr, "\"pop ups/%s/str\" format error!\n", type);
            goto err;
        }

        popup.msg_size = strlen(popup.msg);

        popup.free_msg = true;
    }

    popup.when = SDL_GetTicks();
    (void) editor_add_popup(editor, &popup);

    config_destroy(&cfg);
    return;

err:
    flash_error(editor, "Config error! See program output for more info");
    config_destroy(&cfg);
    return;
}

const char *editor_configured_inline_hint(Editor *editor, const char *type) {
    bool ok;
    const char *str = config_get_str_at(editor->configs.input_hints, type, &ok);
    if (!ok) {
        fprintf(stderr, "\"input hints/%s\" not configured!\n", type);
        flash_error(editor, "Config error! See program output for more info");
        return "???";
    }
    return str;
}

bool editor_load_config(Editor *editor, const char *config_path) {
    config_destroy(&editor->configs.cfg);
    config_init(&editor->configs.cfg);
    allocgroup_free(editor->configs.alloc);

    FILE *file = fopen(config_path, "r");
    if (file == NULL)
        return false;
    editor->configs.alloc = config_add_file(&editor->configs.cfg, file);
    fclose(file);

    {
        Config window;
        config_init(&window);
        bool ok;
        config_child(&window, editor->configs.cfg, "window", &ok);
        if (!ok) {
            fprintf(stderr, "\"window\" not found in config!");
            config_destroy(&window);
            return false;
        }

        editor->configs.window.font = config_get_str_at(window, "font", &ok);
        if (!ok) {
            fprintf(stderr, "\"window/font\" not found in config!");
            config_destroy(&window);
            return false;
        }

        editor->configs.window.title = config_get_str_at(window, "title", &ok);
        if (!ok) {
            fprintf(stderr, "\"window/title\" not found in config!");
            config_destroy(&window);
            return false;
        }

        editor->configs.window.x = config_get_long_at(window, "x", &ok);
        if (!ok) {
            fprintf(stderr, "\"window/x\" not found in config!");
            config_destroy(&window);
            return false;
        }

        editor->configs.window.y = config_get_long_at(window, "y", &ok);
        if (!ok) {
            fprintf(stderr, "\"window/y\" not found in config!");
            config_destroy(&window);
            return false;
        }

        editor->configs.window.w = config_get_long_at(window, "w", &ok);
        if (!ok) {
            fprintf(stderr, "\"window/w\" not found in config!");
            config_destroy(&window);
            return false;
        }

        editor->configs.window.h = config_get_long_at(window, "h", &ok);
        if (!ok) {
            fprintf(stderr, "\"window/h\" not found in config!");
            config_destroy(&window);
            return false;
        }

        config_destroy(&window);
    }

    {
        Config popup;
        config_init(&popup);
        bool ok;
        config_child(&popup, editor->configs.cfg, "pop up", &ok);
        if (!ok) {
            fprintf(stderr, "\"pop up\" not found in config!");
            config_destroy(&popup);
            return false;
        }

        editor->configs.popup.scale = config_get_double_at(popup, "scale", &ok);
        if (!ok) {
            fprintf(stderr, "\"pop up/scale\" not found in config!");
            config_destroy(&popup);
            return false;
        }

        editor->configs.popup.fade_in = config_get_long_at(popup, "fade in", &ok);
        if (!ok) {
            fprintf(stderr, "\"pop up/fade in\" not found in config!");
            config_destroy(&popup);
            return false;
        }

        config_destroy(&popup);
    }

    {
        config_destroy(&editor->configs.popup_messages);
        config_init(&editor->configs.popup_messages);
        bool ok;
        config_child(&editor->configs.popup_messages, editor->configs.cfg, "pop ups", &ok);
        if (!ok) {
            fprintf(stderr, "\"pop ups\" not found in config!");
            return false;
        }
    }

    {
        config_destroy(&editor->configs.input_hints);
        config_init(&editor->configs.input_hints);
        bool ok;
        config_child(&editor->configs.input_hints, editor->configs.cfg, "input hints", &ok);
        if (!ok) {
            fprintf(stderr, "\"input hints\" not found in config!");
            return false;
        }
    }

    return true;
}