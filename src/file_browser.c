#include <string.h>
#include "file_browser.h"
#include "sv.h"

static int file_cmp(const void *ap, const void *bp)
{
    const char *a = *(const char**)ap;
    const char *b = *(const char**)bp;
    return strcmp(a, b);
}

Errno fb_open_dir(File_Browser *fb, const char *dir_path)
{
    fb->files.count = 0;
    fb->cursor = 0;
    Errno err = read_entire_dir(dir_path, &fb->files);
    if (err != 0) {
        return err;
    }
    qsort(fb->files.items, fb->files.count, sizeof(*fb->files.items), file_cmp);

    fb->dir_path.count = 0;
    sb_append_cstr(&fb->dir_path, dir_path);
    sb_append_null(&fb->dir_path);

    return 0;
}

#define PATH_SEP "/"
#define PATH_EMPTY ""
#define PATH_DOT "."
#define PATH_DOTDOT ".."

typedef struct {
    String_View *items;
    size_t count;
    size_t capacity;
} Comps;

void normpath(String_View path, String_Builder *result)
{
    size_t original_sb_size = result->count;

    if (path.count == 0) {
        sb_append_cstr(result, PATH_DOT);
        return;
    }

    int initial_slashes = 0;
    while (path.count > 0 && *path.data == *PATH_SEP) {
        initial_slashes += 1;
        sv_chop_left(&path, 1);
    }
    if (initial_slashes > 2) {
        initial_slashes = 1;
    }

    Comps new_comps = {0};

    while (path.count > 0) {
        String_View comp = sv_chop_by_delim(&path, '/');
        if (comp.count == 0 || sv_eq(comp, SV(PATH_DOT))) {
            continue;
        }
        if (!sv_eq(comp, SV(PATH_DOTDOT))) {
            da_append(&new_comps, comp);
            continue;
        }
        if (initial_slashes == 0 && new_comps.count == 0) {
            da_append(&new_comps, comp);
            continue;
        }
        if (new_comps.count > 0 && sv_eq(da_last(&new_comps), SV(PATH_DOTDOT))) {
            da_append(&new_comps, comp);
            continue;
        }
        if (new_comps.count > 0) {
            new_comps.count -= 1;
            continue;
        }
    }

    for (int i = 0; i < initial_slashes; ++i) {
        sb_append_cstr(result, PATH_SEP);
    }

    for (size_t i = 0; i < new_comps.count; ++i) {
        if (i > 0) sb_append_cstr(result, PATH_SEP);
        sb_append_buf(result, new_comps.items[i].data, new_comps.items[i].count);
    }

    if (original_sb_size == result->count) {
        sb_append_cstr(result, PATH_DOT);
    }

    free(new_comps.items);
}

Errno fb_change_dir(File_Browser *fb)
{
    assert(fb->dir_path.count > 0 && "You need to call fb_open_dir() before fb_change_dir()");
    assert(fb->dir_path.items[fb->dir_path.count - 1] == '\0');

    if (fb->cursor >= fb->files.count) return 0;

    const char *dir_name = fb->files.items[fb->cursor];

    String_Builder new_path = { 0 };
    da_append_many(&new_path, fb->dir_path.items, fb->dir_path.count);

    new_path.count -= 1;

    // TODO: fb->dir_path grows indefinitely if we hit the root
    sb_append_cstr(&new_path, "/");
    sb_append_cstr(&new_path, dir_name);

    String_Builder result = { 0 };
    normpath(sb_to_sv(new_path), &result);
    da_move(&new_path, result);
    sb_append_null(&new_path);

    Files new_files = { 0 };
    Errno err = read_entire_dir(new_path.items, &new_files);
    if (err != 0) {
        return err;
    }

    da_move(&fb->files, new_files);
    da_move(&fb->dir_path, new_path);
    fb->cursor = 0;

    printf("Changed dir to %s\n", fb->dir_path.items);

    qsort(fb->files.items, fb->files.count, sizeof(*fb->files.items), file_cmp);

    return 0;
}

void fb_render(const File_Browser *fb, SDL_Window *window, Free_Glyph_Atlas *atlas, Simple_Renderer *sr)
{
    Vec2f cursor_pos = vec2f(0, -(float)fb->cursor * FREE_GLYPH_FONT_SIZE);

    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    float max_line_len = 0.0f;

    sr->resolution = vec2f(w, h);
    sr->time = (float) SDL_GetTicks() / 1000.0f;

    simple_renderer_set_shader(sr, SHADER_FOR_COLOR);
    if (fb->cursor < fb->files.count) {
        const Vec2f begin = vec2f(0, -((float)fb->cursor + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE);
        Vec2f end = begin;
        free_glyph_atlas_measure_line_sized(
            atlas, fb->files.items[fb->cursor], strlen(fb->files.items[fb->cursor]),
            &end);
        simple_renderer_solid_rect(sr, begin, vec2f(end.x - begin.x, FREE_GLYPH_FONT_SIZE), vec4f(.25, .25, .25, 1));
    }
    simple_renderer_flush(sr);

    simple_renderer_set_shader(sr, SHADER_FOR_EPICNESS);
    for (size_t row = 0; row < fb->files.count; ++row) {
        const Vec2f begin = vec2f(0, -(float)row * FREE_GLYPH_FONT_SIZE);
        Vec2f end = begin;
        free_glyph_atlas_render_line_sized(
            atlas, sr, fb->files.items[row], strlen(fb->files.items[row]),
            &end,
            vec4fs(0));
        // TODO: the max_line_len should be calculated based on what's visible on the screen right now
        float line_len = fabsf(end.x - begin.x);
        if (line_len > max_line_len) {
            max_line_len = line_len;
        }
    }

    simple_renderer_flush(sr);

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

const char *fb_file_path(File_Browser *fb)
{
    assert(fb->dir_path.count > 0 && "You need to call fb_open_dir() before fb_file_path()");
    assert(fb->dir_path.items[fb->dir_path.count - 1] == '\0');

    if (fb->cursor >= fb->files.count) return NULL;

    fb->file_path.count = 0;
    sb_append_buf(&fb->file_path, fb->dir_path.items, fb->dir_path.count - 1);
    sb_append_buf(&fb->file_path, "/", 1);
    sb_append_cstr(&fb->file_path, fb->files.items[fb->cursor]);
    sb_append_null(&fb->file_path);

    return fb->file_path.items;
}
