#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include <SDL2/SDL.h>
#define GLEW_STATIC
#include <GL/glew.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "./editor.h"
#include "./file_browser.h"
#include "./la.h"
#include "./free_glyph.h"
#include "./simple_renderer.h"
#include "./common.h"
#include "./lexer.h"
#include "./sv.h"

// TODO: An ability to create a new file
// TODO: Undo/redo system

void MessageCallback(GLenum source,
                     GLenum type,
                     GLuint id,
                     GLenum severity,
                     GLsizei length,
                     const GLchar* message,
                     const void* userParam)
{
    (void) source;
    (void) id;
    (void) length;
    (void) userParam;
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type, severity, message);
}

static Free_Glyph_Atlas atlas = {0};
static Simple_Renderer sr = {0};
static Editor editor = {0};
static File_Browser fb = {0};

static void onSaveInputPath(Editor *e)
{
    sb_append_null(&e->input.text);
    editor_save_as(e, e->input.text.items);
}

static void onGoToLine(Editor *e)
{
    sb_append_null(&e->input.text);
    const char *linestr = e->input.text.items;
    char *end;
    size_t linenum = strtol(linestr, &end, 10);
    size_t colnum = 0;
    if (*end == ':') {
        char *end2;
        colnum = strtol(end + 1, &end2, 10);
        if (*end2 != '\0')
            goto invalid_input;
    } else if (*end != '\0')
        goto invalid_input;
    if (linenum == 0)
        linenum = 1;
    if (colnum == 0)
        colnum = 1;
    editor_goto(e, linenum - 1, colnum - 1);
    return;

invalid_input:
    flash_error(e, "Invalid line(:col)!");
}

int main(int argc, char **argv)
{
    Errno err;

    FT_Library library = {0};

    FT_Error error = FT_Init_FreeType(&library);
    if (error) {
        fprintf(stderr, "ERROR: Could not initialize FreeType2 library\n");
        return 1;
    }

    // TODO: users should be able to customize the font
    // const char *const font_file_path = "./fonts/VictorMono-Regular.ttf";
    const char *const font_file_path = "./fonts/iosevka-regular.ttf";

    FT_Face face;
    error = FT_New_Face(library, font_file_path, 0, &face);
    if (error == FT_Err_Unknown_File_Format) {
        fprintf(stderr, "ERROR: `%s` has an unknown format\n", font_file_path);
        return 1;
    } else if (error) {
        fprintf(stderr, "ERROR: Could not load file `%s`\n", font_file_path);
        return 1;
    }

    FT_UInt pixel_size = FREE_GLYPH_FONT_SIZE;
    error = FT_Set_Pixel_Sizes(face, 0, pixel_size);
    if (error) {
        fprintf(stderr, "ERROR: Could not set pixel size to %u\n", pixel_size);
        return 1;
    }

    if (argc > 1) {
        const char *file_path = argv[1];
        err = editor_load_from_file(&editor, file_path);
        if (err != 0) {
            fprintf(stderr, "ERROR: Could not read file %s: %s\n", file_path, strerror(err));
            return 1;
        }
    }

    const char *dir_path = ".";
    err = fb_open_dir(&fb, dir_path);
    if (err != 0) {
        fprintf(stderr, "ERROR: Could not read directory %s: %s\n", dir_path, strerror(err));
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "ERROR: Could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window =
        SDL_CreateWindow("ded",
                         100, 100,
                         SCREEN_WIDTH, SCREEN_HEIGHT,
                         SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (window == NULL) {
        fprintf(stderr, "ERROR: Could not create SDL window: %s\n", SDL_GetError());
        return 1;
    }

    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        int major;
        int minor;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
        printf("GL version %d.%d\n", major, minor);
    }

    if (SDL_GL_CreateContext(window) == NULL) {
        fprintf(stderr, "ERROR: Could not create OpenGL context: %s\n", SDL_GetError());
        return 1;
    }

    GLenum glewErr = glewInit();
    if (GLEW_OK != glewErr) {
        fprintf(stderr, "ERROR: Could not initialize GLEW: %s\n", glewGetErrorString(glewErr));
        return 1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (GLEW_ARB_debug_output) {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback((GLDEBUGPROC) MessageCallback, 0);
    } else {
        fprintf(stderr, "WARNING: GLEW_ARB_debug_output is not available");
    }

    simple_renderer_init(&sr);
    free_glyph_atlas_init(&atlas, face);

    editor.atlas = &atlas;
    editor_retokenize(&editor);

    bool is_fullscreen = false;
    bool quit = false;
    bool file_browser = false;
    while (!quit) {
        const Uint32 start = SDL_GetTicks();
        SDL_Event event = {0};
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: {
                quit = true;
            }
            break;

            case SDL_KEYDOWN: {
                switch (event.key.keysym.sym) {
                    case SDLK_F11: {
                        is_fullscreen = !is_fullscreen;
                        SDL_SetWindowFullscreen(window, is_fullscreen * SDL_WINDOW_FULLSCREEN_DESKTOP);
                    }
                    break;
                }

                if (file_browser) {
                    switch (event.key.keysym.sym) {
                    case SDLK_UP: {
                        if (fb.cursor > 0) fb.cursor -= 1;
                    }
                    break;

                    case SDLK_DOWN: {
                        if (fb.cursor + 1 < fb.files.count) fb.cursor += 1;
                    }
                    break;

                    case SDLK_RETURN: {
                        const char *file_path = fb_file_path(&fb);
                        if (file_path) {
                            File_Type ft;
                            err = type_of_file(file_path, &ft);
                            if (err != 0) {
                                flash_error(&editor, "Could not determine type of file %s: %s", file_path, strerror(err));
                            } else {
                                switch (ft) {
                                case FT_DIRECTORY: {
                                    err = fb_change_dir(&fb);
                                    if (err != 0) {
                                        flash_error(&editor, "Could not change directory to %s: %s", file_path, strerror(err));
                                    }
                                }
                                break;

                                case FT_REGULAR: {
                                    err = editor_load_from_file(&editor, file_path);
                                    if (err != 0) {
                                        flash_error(&editor, "Could not open file %s: %s", file_path, strerror(err));
                                    } else {
                                        file_browser = false;
                                    }
                                }
                                break;

                                case FT_OTHER: {
                                    flash_error(&editor, "%s is neither a regular file nor a directory. We can't open it.", file_path);
                                }
                                break;

                                default:
                                    UNREACHABLE("unknown File_Type");
                                }
                            }
                        }
                    }
                    break;
                    }
                } else {
                    switch (event.key.keysym.sym) {
                    case SDLK_HOME: {
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            editor_move_to_begin(&editor);
                        } else {
                            editor_move_to_line_begin(&editor);
                        }
                    } break;

                    case SDLK_END: {
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            editor_move_to_end(&editor);
                        } else {
                            editor_move_to_line_end(&editor);
                        }
                    } break;

                    case SDLK_BACKSPACE: {
                        if (editor.selection) {
                            editor_delete_selection(&editor);
                            editor.selection = false;
                        } else {
                            editor_backspace(&editor);
                        }
                    }
                    break;

                    case SDLK_s: {
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            if (event.key.keysym.mod & KMOD_SHIFT) {
                                editor_start_input(&editor);
                                editor.input.onDone = onSaveInputPath;
                                editor.input.hint = "path: ";
                                editor.input.hint_len = strlen(editor.input.hint);
                            } else if (editor.file_path.count > 0) {
                                err = editor_save(&editor);
                                if (err != 0) {
                                    flash_error(&editor, "Can't save: %s", strerror(err));
                                }
                            } else {
                                editor_start_input(&editor);
                                editor.input.onDone = onSaveInputPath;
                                editor.input.required = true;
                                editor.input.hint = "path: ";
                                editor.input.hint_len = strlen(editor.input.hint);
                            }
                        }
                    }
                    break;

                    case SDLK_o: {
                        // TODO: annoy user if unsaved changes
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            file_browser = true;
                        }
                    }
                    break;

                    case SDLK_F5: {
                        simple_renderer_reload_shaders(&sr);
                    }
                    break;

                    case SDLK_RETURN: {
                        if (editor.searching) {
                            editor_stop_search(&editor);
                        } else if (editor.input.active) {
                            editor.input.active = false;
                            if (editor.input.onDone)
                                editor.input.onDone(&editor);
                        } else {
                            if (editor.selection) {
                                editor_delete_selection(&editor);
                                editor.selection = false;
                            }
                            editor_insert_char(&editor, '\n');
                        }
                    }
                    break;

                    case SDLK_DELETE: {
                        if (editor.selection) {
                            editor_delete_selection(&editor);
                            editor.selection = false;
                        } else {
                            editor_delete(&editor);
                        }
                    }
                    break;

                    case SDLK_f: {
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            editor_start_search(&editor);
                        }
                    }
                    break;

                    case SDLK_ESCAPE: {
                        editor_stop_search(&editor);
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                    }
                    break;

                    case SDLK_a: {
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            editor.selection = true;
                            editor.select_begin = 0;
                            editor.cursor = editor.data.count;
                        }
                    }
                    break;

                    case SDLK_TAB: {
                        if (editor.searching) {
                            editor_start_search(&editor);
                        } else {
                            // TODO: indent on Tab instead of just inserting 4 spaces at the cursor
                            // That is insert the spaces at the beginning of the line. Shift+TAB should
                            // do unindent, that is remove 4 spaces from the beginning of the line.
                            // TODO: customizable indentation style
                            // - tabs/spaces
                            // - tab width
                            // - etc.
                            for (size_t i = 0; i < 4; ++i) {
                                editor_insert_char(&editor, ' ');
                            }
                        }
                    }
                    break;

                    case SDLK_c: {
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            editor_clipboard_copy(&editor);
                        }
                    }
                    break;

                    case SDLK_x: {
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            editor_clipboard_copy(&editor);
                            if (editor.selection) {
                                editor_delete_selection(&editor);
                                editor.selection = false;
                            }
                        }
                    }
                    break;

                    case SDLK_v: {
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            editor_clipboard_paste(&editor);
                        }
                    }
                    break;

                    case SDLK_g: {
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            editor_start_input(&editor);
                            editor.input.onDone = onGoToLine;
                            editor.input.hint = "line: ";
                            editor.input.hint_len = strlen(editor.input.hint);
                        }
                    }
                    break;

                    case SDLK_UP: {
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            editor_move_paragraph_up(&editor);
                        } else {
                            editor_move_line_up(&editor);
                        }
                    }
                    break;

                    case SDLK_DOWN: {
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            editor_move_paragraph_down(&editor);
                        } else {
                            editor_move_line_down(&editor);
                        }
                    }
                    break;

                    case SDLK_LEFT: {
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            editor_move_word_left(&editor);
                        } else {
                            editor_move_char_left(&editor);
                        }
                    }
                    break;

                    case SDLK_RIGHT: {
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            editor_move_word_right(&editor);
                        } else {
                            editor_move_char_right(&editor);
                        }
                    }
                    break;
                    }

                    editor.last_stroke = SDL_GetTicks();
                }
            }
            break;

            case SDL_WINDOWEVENT: {
                switch(event.window.event) {
                case SDL_WINDOWEVENT_RESIZED: {
                    int w, h;
                    SDL_GetWindowSize(window, &w, &h);
                    glViewport(0, 0, w, h);
                  }
                break;
                }
            }
            break;

            case SDL_TEXTINPUT: {
                if (editor.input.active) {
                    sb_append_cstr(&editor.input.text, event.text.text);
                } else if (file_browser) {
                    // Nothing for now
                    // Once we have incremental search in the file browser this may become useful
                } else {
                    const char *text = event.text.text;
                    size_t text_len = strlen(text);
                    for (size_t i = 0; i < text_len; ++i) {
                        editor_insert_char(&editor, text[i]);
                    }
                    editor.last_stroke = SDL_GetTicks();
                }
                if (editor.searching && !editor_search_matches_at(&editor, editor.cursor)) {
                    editor_start_search(&editor);
                }
            }
            break;
            }
        }

        Vec4f bg = hex_to_vec4f(0x181818FF);
        glClearColor(bg.x, bg.y, bg.z, bg.w);
        glClear(GL_COLOR_BUFFER_BIT);

        if (file_browser) {
            fb_render(&fb, window, &atlas, &sr);
        } else {
            editor_render(window, &atlas, &sr, &editor);
        }

        SDL_GL_SwapWindow(window);

        const Uint32 duration = SDL_GetTicks() - start;
        const Uint32 delta_time_ms = 1000 / FPS;
        if (duration < delta_time_ms) {
            SDL_Delay(delta_time_ms - duration);
        }
    }

    return 0;
}

// TODO: ability to search within file browser
// Very useful when you have a lot of files
// TODO: ability to remove trailing whitespaces
