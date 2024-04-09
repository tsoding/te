#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include "common.h"
#include "helix.h"

#include <dirent.h>

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

// added
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include "yasnippet.h"
#include "render.h"
#include "evil.h"
#include "emacs.h"
#include "buffer.h"
#include "theme.h"
#include "unistd.h"
#include "M-x.h"
#include "lsp.h"
#include "clock.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define FONT_DIR "~/.config/ded/fonts/"
/* #define DEFAULT_FONT "jet-extra-bold.ttf" */
#define DEFAULT_FONT "radon.otf"
/* #define DEFAULT_FONT "minecraft.ttf" */
/* #define DEFAULT_FONT "iosevka-regular.ttf" */
#define MAX_FONTS 20
#define MAX_PATH_SIZE 1024

char *fonts[MAX_FONTS];
int font_count = 0;
int current_font_index = 0;


// TODO: Save file dialog
// Needed when ded is ran without any file so it does not know where to save.

// TODO: An ability to create a new file
// TODO: Undo/redo system
// DONE: Delete a word
// DONE: Delete selection
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


FT_Face load_font_face(FT_Library library, const char *font_name, FT_UInt pixel_size) {
    printf("Loading font: %s at index: %d\n", font_name, current_font_index);
    char font_path[MAX_PATH_SIZE];
    const char *homeDir = getenv("HOME");
    snprintf(font_path, sizeof(font_path), "%s/.config/ded/fonts/%s", homeDir, font_name);

    FT_Face face;
    FT_Error error = FT_New_Face(library, font_path, 0, &face);
    if (error == FT_Err_Unknown_File_Format) {
        fprintf(stderr, "ERROR: `%s` has an unknown format\n", font_path);
        exit(1);
    } else if (error) {
        fprintf(stderr, "ERROR: Could not load file `%s`\n", font_path);
        exit(1);
    }

    error = FT_Set_Pixel_Sizes(face, 0, pixel_size); // Set pixel size for the loaded font face
    if (error) {
        fprintf(stderr, "ERROR: Could not set pixel size to %u\n", pixel_size);
        return NULL; // or handle the error in a different way
    }

    return face;
}

void prev_font() {
    if (current_font_index == 0) {
        // Already at the first font, don't do anything.
        return;
    }
    current_font_index--;
}

void next_font() {
    if (current_font_index == font_count - 1) {
        // Already at the last font, don't do anything.
        return;
    }
    current_font_index++;
}


void populate_font_list() {
    char path[MAX_PATH_SIZE];
    const char *homeDir = getenv("HOME");
    if (!homeDir) {
        fprintf(stderr, "ERROR: Could not get HOME directory\n");
        exit(1);
    }

    snprintf(path, sizeof(path), "%s/.config/ded/fonts/", homeDir);

    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "ERROR: Could not open directory `%s`\n", path);
        exit(1);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) && font_count < MAX_FONTS) {
        if (entry->d_type == DT_REG) {  // If the entry is a regular file
            fonts[font_count] = strdup(entry->d_name);
            font_count++;
        }
    }
    closedir(dir);
}

void switch_to_font(FT_Library library, FT_Face *currentFace, Free_Glyph_Atlas *atlas, int direction) {
    if (direction > 0) {
        next_font();
    } else {
        prev_font();
    }
    /* *currentFace = load_font_face(library, fonts[current_font_index]); */
    *currentFace = load_font_face(library, fonts[current_font_index], FREE_GLYPH_FONT_SIZE);


    // Dispose the old texture
    /* glDeleteTextures(1, &atlas->glyphs_texture); */

    // Reinitialize the atlas with the new font face
    free_glyph_atlas_init(atlas, *currentFace);
}

// TODO: display errors reported via flash_error right in the text editor window somehow
#define flash_error(...) do { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)

#include <signal.h>

int main(int argc, char **argv)
{
    set_current_mode();
    initialize_themes();
    initialize_shader_paths();
    load_snippets_from_directory();

    init_clock();
    
    // lsp
    start_clangd(&editor);
    /* pthread_create(&receive_thread, NULL, receive_json_rpc, NULL); */


    // Define hash seeds (these could be randomly generated for more robustness)
    uint64_t seed0 = 0x12345678;
    uint64_t seed1 = 0x9ABCDEF0;

    // Allocate and initialize the commands hashmap
    editor.commands = hashmap_new(
        sizeof(Command), // Size of each element
        16,              // Initial capacity
        seed0, seed1,    // Hash seeds
        simple_string_hash,     // Hash function
        command_compare, // Compare function (you need to define this based on your Command struct)
        NULL,            // Element free function (NULL if not needed)
        NULL             // User data for compare function (NULL if not needed)
    );

    if (!editor.commands) {
        // Handle allocation failure
        fprintf(stderr, "Failed to initialize command map\n");
        return -1;
    }

    initialize_commands(editor.commands);

    
    Errno err;

    FT_Library library = {0};

    FT_Error error = FT_Init_FreeType(&library);
    if (error) {
        fprintf(stderr, "ERROR: Could not initialize FreeType2 library\n");
        return 1;
    }

    // TODO: users should be able to customize the font
    /* const char *const font_file_path = "./fonts/VictorMono-Regular.ttf"; */
    /* const char *const font_file_path = "./fonts/jet-bold.ttf"; */
    /* const char *const font_file_path = "~/.config/ded/fonts/jet-extra-bold.ttf"; */
    /* const char *const font_file_path = "./fonts/iosevka-regular.ttf"; */


    /* char font_file_path_buffer[1024]; */
    /* const char *homeDir = getenv("HOME"); */
    /* if (homeDir) { */
    /*   snprintf(font_file_path_buffer, sizeof(font_file_path_buffer), "%s/.config/ded/fonts/minecraft_font.ttf", homeDir); */
    /* } else { */
    /*   // handle the error, for now, we'll just set it to the original value as a fallback */
    /*   strncpy(font_file_path_buffer, "~/.config/ded/fonts/jet-extra-bold.ttf", sizeof(font_file_path_buffer)); */
    /* } */
    /* const char *const font_file_path = font_file_path_buffer; */



    populate_font_list();

    if (font_count == 0) {
      fprintf(stderr, "ERROR: No fonts found in `%s`\n", FONT_DIR);
      return 1;
    }

    // Start with the default font
    for (int i = 0; i < font_count; i++) {
      if (strcmp(fonts[i], DEFAULT_FONT) == 0) {
        current_font_index = i;
        break;
      }
    }

    /* FT_Face face = load_font_face(library, fonts[current_font_index]); */
    FT_Face face = load_font_face(library, fonts[current_font_index], FREE_GLYPH_FONT_SIZE);

    
    /* original */
    /* FT_Face face; */
    /* error = FT_New_Face(library, font_file_path, 0, &face); */
    /* if (error == FT_Err_Unknown_File_Format) { */
    /*     fprintf(stderr, "ERROR: `%s` has an unknown format\n", font_file_path); */
    /*     return 1; */
    /* } else if (error) { */
    /*     fprintf(stderr, "ERROR: Could not load file `%s`\n", font_file_path); */
    /*     return 1; */
    /* } */


    FT_UInt pixel_size = FREE_GLYPH_FONT_SIZE;
    error = FT_Set_Pixel_Sizes(face, 0, pixel_size);
    if (error) {
        fprintf(stderr, "ERROR: Could not set pixel size to %u\n", pixel_size);
        return 1;
    }

    if (argc > 1) {
        const char *file_path = argv[1];
        err = find_file(&editor, file_path, 0, 0);
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
                         0, 0,
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
        glDebugMessageCallback(MessageCallback, 0);
    } else {
        fprintf(stderr, "WARNING: GLEW_ARB_debug_output is not available");
    }

    simple_renderer_init(&sr);
    free_glyph_atlas_init(&atlas, face);

    editor.atlas = &atlas;
    editor_retokenize(&editor);

    
    /* bool quit = false; */
    bool file_browser = false;

    while (!quit) {
        const Uint32 start = SDL_GetTicks();
        SDL_Event event = {0};
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
              quit = true;
              break;

            case SDL_KEYDOWN:
                if (current_mode == NORMAL) {
                    if (handle_evil_find_char(&editor, &event)) {
                        break; // Skip further processing if the key event was handled
                    }
                }

              if (file_browser) {
                switch (event.key.keysym.sym) {
                case SDLK_F3: {
                  file_browser = false;
                } break;

                case SDLK_RETURN: {
                    const char *file_path = fb_file_path(&fb);
                    if (file_path) {
                        File_Type ft;
                        err = type_of_file(file_path, &ft);
                        if (err != 0) {
                            flash_error("Could not determine type of file %s: %s", file_path, strerror(err));
                        } else {
                            switch (ft) {
                            case FT_DIRECTORY: {
                                err = fb_change_dir(&fb);
                                if (err != 0) {
                                    flash_error("Could not change directory to %s: %s", file_path, strerror(err));
                                }
                            }
                            break;

                                case FT_REGULAR: {
                                    // TODO: before opening a new file make sure you don't have unsaved changes
                                    // And if you do, annoy the user about it. (just like all the other editors do)
                                    err = find_file(&editor, file_path, 0, 0);
                                    if (err != 0) {
                                        flash_error("Could not open file %s: %s", file_path, strerror(err));
                                    } else {
                                        file_browser = false;
                                    }
                                }
                                break;

                                case FT_OTHER: {
                                    flash_error("%s is neither a regular file nor a directory. We can't open it.", file_path);
                                }
                                break;

                                default:
                                    UNREACHABLE("unknown File_Type");
                                }
                            }
                        }
                    }
                    break;

                case SDLK_EQUALS: {
                  if (SDL_GetModState() & KMOD_ALT) {  // Check if ALT is pressed
                    theme_next(&currentThemeIndex);
                    printf("Changed theme to %d\n", currentThemeIndex); // Logging the theme change for debugging
                  } else if (SDL_GetModState() & KMOD_CTRL) {  // Check if CTRL is pressed
                      zoom_factor -= 0.8f;
                    if (zoom_factor < min_zoom_factor) {
                      zoom_factor = min_zoom_factor;
                    }
                  }
                } break;

                case SDLK_MINUS: {
                  if (SDL_GetModState() & KMOD_ALT) {  // Check if ALT is pressed
                    theme_previous(&currentThemeIndex);
                    printf("Changed theme back to %d\n", currentThemeIndex); // Logging the theme change for debugging
                  } else if (SDL_GetModState() & KMOD_CTRL) {  // Check if CTRL is pressed
                    zoom_factor += 0.8f;
                    if (zoom_factor > max_zoom_factor) {
                      zoom_factor = max_zoom_factor;
                    }
                  }
                } break;

                case SDLK_q:
                case SDLK_ESCAPE: {
                  file_browser = false;
                } break;


                case SDLK_r:
                  if (event.key.keysym.mod & KMOD_CTRL) {
                    file_browser = false;
                  }
                  break;


                case SDLK_F5: {
                  simple_renderer_reload_shaders(&sr);
                }
                  break;

                case SDLK_UP: {
                  if (fb.cursor > 0)
                    fb.cursor -= 1;
                } break;

                case SDLK_k: {
                  if (fb.cursor > 0)
                    fb.cursor -= 1;
                } break;

                case SDLK_DOWN: {
                  if (fb.cursor + 1 < fb.files.count)
                    fb.cursor += 1;
                } break;

                case SDLK_j: {
                  if (fb.cursor + 1 < fb.files.count)
                    fb.cursor += 1;
                } break;

                 // TODO cant go back more than the original direcory
                case SDLK_h: {
                  // Copy current directory path
                  char current_dir[PATH_MAX];
                  strncpy(current_dir, fb.dir_path.items, fb.dir_path.count);
                  current_dir[fb.dir_path.count - 1] = '\0'; // Ensure null-termination

                  // Get parent directory
                  char *parent = dirname(current_dir);

                  // Open parent directory
                  Errno err = fb_open_dir(&fb, parent);
                  if (err != 0) {
                    // Handle error, for example, print out an error message.
                  } else {
                    fb.cursor = 0; // Reset cursor position in the new directory
                  }
                } break;

                case SDLK_t: {
                  if (SDL_GetModState() & KMOD_CTRL) {
                    followCursor = !followCursor;  // Toggle the state
                  }
                }
                  break;


                case SDLK_l: {
                  const char *file_path = fb_file_path(&fb);
                  if (file_path) {
                    File_Type ft;
                    err = type_of_file(file_path, &ft);
                    if (err != 0) {
                      flash_error("Could not determine type of file %s: %s",
                                  file_path, strerror(err));
                    } else {
                      switch (ft) {
                      case FT_DIRECTORY: {
                        err = fb_change_dir(&fb);
                        if (err != 0) {
                          flash_error("Could not change directory to %s: %s",
                                      file_path, strerror(err));
                        }
                      } break;

                      case FT_REGULAR: {
                        // TODO: before opening a new file make sure you don't
                        // have unsaved changes And if you do, annoy the user
                        // about it. (just like all the other editors do)
                        err = find_file(&editor, file_path, 0, 0);
                        if (err != 0) {
                          flash_error("Could not open file %s: %s", file_path,
                                      strerror(err));
                        } else {
                          file_browser = false;
                        }
                      } break;

                      case FT_OTHER: {
                        flash_error("%s is neither a regular file nor a "
                                    "directory. We can't open it.",
                                    file_path);
                      } break;

                      default:
                        UNREACHABLE("unknown File_Type");
                      }
                    }
                  }
                }
                break;
                }
              } else {
                switch (current_mode) {
                case EMACS:
                    // TODO add all keybinds
                    switch (event.key.keysym.sym) {

                    
                    case SDLK_z: {
                        if (SDL_GetModState() & KMOD_CTRL) {
                            current_mode = NORMAL;
                            editor.last_stroke = SDL_GetTicks();
                        }
                    }
                    break;

                    
                    case SDLK_BACKSPACE:
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            emacs_backward_kill_word(&editor);
                            editor.last_stroke = SDL_GetTicks();
                        }else{
                            editor_backspace(&editor);
                            editor.last_stroke = SDL_GetTicks();
                    }
                    break;
                        

                    case SDLK_t: {
                        if (SDL_GetModState() & KMOD_CTRL) {
                            followCursor = !followCursor;
                        }
                    }
                    break;

                    case SDLK_TAB: {
                        activate_snippet(&editor);
                        editor.last_stroke = SDL_GetTicks();
                    }
                    break;
                      
                    case SDLK_r:
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            file_browser = true;
                        }
                        break;
                        
                    case SDLK_n: {
                        if (SDL_GetModState() & KMOD_CTRL) {
                            editor_move_line_down(&editor);
                            editor.last_stroke = SDL_GetTicks();
                        }
                    } break;
                        
                    case SDLK_p:
                        if (SDL_GetModState() & KMOD_CTRL){

                            editor_move_line_up(&editor);
                        }
                        editor.last_stroke = SDL_GetTicks();
                        break;


                    case SDLK_v:
                        if (SDL_GetModState() & KMOD_CTRL){
                            editor_clipboard_paste(&editor);
                        }
                        editor.last_stroke = SDL_GetTicks();
                        break;

                        
                    case SDLK_b:
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                        if (SDL_GetModState() & KMOD_CTRL){
                            editor_move_char_left(&editor);
                        } else {
                            editor_move_word_left(&editor);
                        }
                        editor.last_stroke = SDL_GetTicks();
                        break;


                    case SDLK_RETURN: {
                        editor_enter(&editor);
                    }
                    break;

                    
                    case SDLK_EQUALS: {
                        if (SDL_GetModState() & KMOD_ALT) {  // Check if ALT is pressed
                            theme_next(&currentThemeIndex);
                            printf("Changed theme to %d\n", currentThemeIndex); // Logging the theme change for debugging
                        } else if (SDL_GetModState() & KMOD_CTRL) {  // Check if CTRL is pressed
                            zoom_factor -= 1.0f;
                            if (zoom_factor < min_zoom_factor) {
                                zoom_factor = min_zoom_factor;
                            }
                            printf("zoom_factor: %.6f", zoom_factor);
                        }
                    } break;

                    case SDLK_MINUS: {
                        if (SDL_GetModState() & KMOD_ALT) {  // Check if ALT is pressed
                            theme_previous(&currentThemeIndex);
                            printf("Changed theme back to %d\n", currentThemeIndex); // Logging the theme change for debugging
                        } else if (SDL_GetModState() & KMOD_CTRL) {  // Check if CTRL is pressed
                            zoom_factor += 1.0f;
                            if (zoom_factor > max_zoom_factor) {
                                zoom_factor = max_zoom_factor;
                            }
                            printf("zoom_factor: %.6f", zoom_factor);
                        }
                    } break;
                        
                    case SDLK_f:
                        if (SDL_GetModState() & KMOD_CTRL) {
                            editor_move_char_right(&editor);
                        }
                        editor.last_stroke = SDL_GetTicks();
                        break;
                        
                    case SDLK_s: {
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            editor_start_search(&editor);
                        }
                    }}
                    break;
                                        
                case NORMAL:
                    switch (event.key.keysym.sym) {
                    SDL_Event tmpEvent; // Declare once at the beginning of the switch block

                    case SDLK_RETURN: {
                        if (!toggle_bool(&editor)) {
                            editor_open_include(&editor);
                        }
                    } break;


                    case SDLK_SEMICOLON:
                        if (event.key.keysym.mod & KMOD_SHIFT) {
                            current_mode = MINIBUFFER;
                            evil_command_active = true;
                            editor.minibuffer_active = true;
                            
                            // Consume the next SDL_TEXTINPUT event for ':'
                            SDL_Event tmpEvent;
                            SDL_PollEvent(&tmpEvent);
                            if (tmpEvent.type != SDL_TEXTINPUT || tmpEvent.text.text[0] != ':') {
                                SDL_PushEvent(&tmpEvent); // Push the event back if it's not the one we're trying to consume
                            }
                        }
                        break;


                        
                    case SDLK_d:
                        if (event.key.keysym.mod & KMOD_SHIFT) {
                            emacs_kill_line(&editor);
                        } else if (editor.selection) {
                            editor_clipboard_copy(&editor);
                            editor_delete_selection(&editor);
                            editor.selection = false;
                        } else if (event.key.keysym.mod & KMOD_CTRL) {
                            evil_delete_char(&editor);
                        } else {
                            emacs_kill_line(&editor);
                        }
                    break;
                                         
                    case SDLK_c:
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            centeredText = !centeredText;
                        } else if (event.key.keysym.mod & KMOD_ALT) {
                            instantCamera = !instantCamera;
                        } else if (event.key.keysym.mod & KMOD_SHIFT) {
                            evil_change_line(&editor);

                            // Eat up the next SDL_TEXTINPUT event for 'C'
                            SDL_PollEvent(&tmpEvent);
                            if (tmpEvent.type != SDL_TEXTINPUT ||
                                (tmpEvent.text.text[0] != 'C')) {
                                SDL_PushEvent(&tmpEvent); // Push it back to the event queue if it's not
                            }
                        }                            
                        break;

                    case SDLK_m:
                        if (event.key.keysym.mod & KMOD_ALT) {
                            emacs_back_to_indentation(&editor);
                        }
                        break;

                    case SDLK_ESCAPE: {
                        if (fzy) {
                            minibufferHeight -= 189;
                            fzy = false;
                        }

                        if (editor.minibuffer_active) {
                            M_x_active = false;
                            editor.minibuffer_active = false;
                        }

                        mixSelectionColor = false ;
                        editor_clear_mark(&editor);
                        editor_stop_search(&editor);
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                    }
                    break;

                    case SDLK_SPACE: {
                        if (SDL_GetModState() & KMOD_CTRL) {
                            if (!editor.has_anchor){
                                editor_set_anchor(&editor);
                            } else {
                                editor_goto_anchor_and_clear(&editor);
                            }
                        } else if (!fzy) {
                            // TODO time delay whichkey
                            minibufferHeight += 189;
                            fzy = true;
                        }
                    }
                    break;

                    case SDLK_5: {
                        if (SDL_GetModState() & KMOD_SHIFT) {
                            evil_jump_item(&editor);
                        }
                    }
                    break;

                    case SDLK_8: {
                        if (SDL_GetModState() & KMOD_SHIFT) {
                            evil_search_word_forward(&editor);
                        }
                    }
                    break;


                    case SDLK_1: {
                        if (SDL_GetModState() & KMOD_CTRL) {
                            hl_line = !hl_line;
                        }
                    }
                    break;


                    case SDLK_o:
                        if (SDL_GetModState() & KMOD_SHIFT) {
                            evil_open_above(&editor);
                        } else {
                            evil_open_below(&editor);
                        }
                        
                        current_mode = INSERT;
                        editor.last_stroke = SDL_GetTicks();
                        
                        // Eat up the next SDL_TEXTINPUT event for 'o' or 'O'
                        SDL_PollEvent(&tmpEvent);
                        if (tmpEvent.type != SDL_TEXTINPUT ||
                            (tmpEvent.text.text[0] != 'o' && tmpEvent.text.text[0] != 'O')) {
                            SDL_PushEvent(&tmpEvent); // Push it back to the event queue if it's not
                        }
                        break;

                  case SDLK_LEFTBRACKET:
                    if (SDL_GetModState() & KMOD_ALT) {
                      switch_to_font(library, &face, &atlas, -1);
                      printf("Switched to previous font: %s\n", fonts[current_font_index]);
                      /* redraw_screen(); */
                    }
                    break;

                  case SDLK_RIGHTBRACKET:
                    if (SDL_GetModState() & KMOD_ALT) {
                      switch_to_font(library, &face, &atlas, 1);
                      printf("Switched to next font: %s\n", fonts[current_font_index]);
                      /* redraw_screen(); */
                    }
                    break;


                    case SDLK_TAB: {
                        indent(&editor);
                    }
                    break;

                    case SDLK_z: {
                        if (SDL_GetModState() & KMOD_CTRL) {
                            /* helix_mode(); */
                            current_mode = EMACS;
                            editor.last_stroke = SDL_GetTicks();
                        }
                    }
                    break;

                  case SDLK_t: {
                    if (SDL_GetModState() & KMOD_CTRL) {
                      followCursor = !followCursor;
                    }
                  }
                    break;


                    case SDLK_F5: {
                        simple_renderer_reload_shaders(&sr);
                    }
                    break;

                    case SDLK_y:
                        if (editor.selection) {
                            editor_clipboard_copy(&editor);
                        } else {
                            evil_yank_line(&editor);
                        }
                        break;

                    
                  case SDLK_g: {
                    if (SDL_GetModState() & KMOD_SHIFT) {
                      editor_move_to_end(&editor);
                    } else {
                      editor_move_to_begin(&editor);
                    }
                  } break;

                  case SDLK_SLASH: {
                    current_mode = MINIBUFFER;
                    editor.last_stroke = SDL_GetTicks();
                    editor_start_search(&editor);

                    // Consume the next SDL_TEXTINPUT event for '/'
                    SDL_Event tmpEvent;
                    SDL_PollEvent(&tmpEvent);
                    if (tmpEvent.type != SDL_TEXTINPUT || tmpEvent.text.text[0] != '/') {
                      SDL_PushEvent(&tmpEvent); // Push the event back if it's not the one we're trying to consume
                    }
                  } break;

                    case SDLK_n: {
                        if (SDL_GetModState() & KMOD_CTRL) {
                            editor_move_line_down(&editor);
                        } else if (SDL_GetModState() & KMOD_ALT) {
                            editor_next_buffer(&editor);
                        } else if (SDL_GetModState() & KMOD_SHIFT) {
                            evil_search_previous(&editor);
                        } else {
                            evil_search_next(&editor);
                        }
                    } break;
                        
                    case SDLK_p:
                        if (SDL_GetModState() & KMOD_CTRL){
                            editor_move_line_up(&editor);
                        } else if (SDL_GetModState() & KMOD_ALT) {
                            editor_previous_buffer(&editor);
                        } else if (copiedLine) {
                            if (SDL_GetModState() & KMOD_SHIFT) {
                                evil_paste_before(&editor);
                            } else {
                                evil_paste_after(&editor);
                            }
                        } else {
                            editor_clipboard_paste(&editor);
                        }
                        break;
                        
                  case SDLK_b:
                    editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                    if (SDL_GetModState() & KMOD_CTRL){
                      editor_move_char_left(&editor);
                    } else if (SDL_GetModState() & KMOD_ALT) {
                        editor_kill_buffer(&editor);
                    } else {
                        editor_move_word_left(&editor);
                    }
                    break;

                  case SDLK_f:
                    if (SDL_GetModState() & KMOD_CTRL) {
                        followCursor = !followCursor;
                        /* editor_move_char_right(&editor); */
                    }
                    break;

                    case SDLK_s: {
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            if (ctrl_x_pressed) {
                                editor_save(&editor);
                            } else {
                                editor_start_search(&editor);
                                current_mode = MINIBUFFER;
                            }
                        } else {
                            // Either S or Shift+S is pressed
                            if (event.key.keysym.mod & KMOD_SHIFT) {
                                evil_change_whole_line(&editor);
                            } else {
                                evil_substitute(&editor);
                            }
                            editor.selection = false;
                            // Eat up the next SDL_TEXTINPUT event for 's' or 'S'
                            SDL_PollEvent(&tmpEvent);
                            if (tmpEvent.type != SDL_TEXTINPUT ||
                                (tmpEvent.text.text[0] != 's' && tmpEvent.text.text[0] != 'S')) {
                                SDL_PushEvent(&tmpEvent); // Push it back to the event queue if it's not
                            }
                            editor.last_stroke = SDL_GetTicks();
                        }
                        break;
                    }
                        
                  case SDLK_EQUALS: {
                    if (SDL_GetModState() & KMOD_ALT) {  // Check if ALT is pressed
                      theme_next(&currentThemeIndex);
                      printf("Changed theme to %d\n", currentThemeIndex); // Logging the theme change for debugging
                    } else if (SDL_GetModState() & KMOD_CTRL) {  // Check if CTRL is pressed
                      zoom_factor -= 1.0f;
                      if (zoom_factor < min_zoom_factor) {
                        zoom_factor = min_zoom_factor;
                      }
                      printf("zoom_factor: %.6f", zoom_factor);
                    }
                  } break;

                  case SDLK_MINUS: {
                    if (SDL_GetModState() & KMOD_ALT) {  // Check if ALT is pressed
                      theme_previous(&currentThemeIndex);
                      printf("Changed theme back to %d\n", currentThemeIndex); // Logging the theme change for debugging
                    } else if (SDL_GetModState() & KMOD_CTRL) {  // Check if CTRL is pressed
                      zoom_factor += 1.0f;
                      if (zoom_factor > max_zoom_factor) {
                        zoom_factor = max_zoom_factor;
                      }
                      printf("zoom_factor: %.6f", zoom_factor);
                    }
                  } break;

                    case SDLK_i:
                        if (SDL_GetModState() & KMOD_CTRL) {
                            showIndentationLines = !showIndentationLines;
                        } else if (SDL_GetModState() & KMOD_ALT) {
                            if (SDL_GetModState() & KMOD_SHIFT) {
                                remove_one_indentation(&editor);
                            } else {
                                add_one_indentation(&editor);
                            }
                        } else {
                            if (SDL_GetModState() & KMOD_SHIFT) {
                                evil_insert_line(&editor);
                            } else {
                                current_mode = INSERT;
                            }

                            editor.last_stroke = SDL_GetTicks();
                            
                            // Eat up the next SDL_TEXTINPUT event for 'i' or 'I'
                            SDL_PollEvent(&tmpEvent);
                            if (tmpEvent.type != SDL_TEXTINPUT ||
                                (tmpEvent.text.text[0] != 'i' && tmpEvent.text.text[0] != 'I')) {
                                SDL_PushEvent(&tmpEvent); // Push it back to the event queue if it's not
                            }
                        }
                        break;
                                          
                  case SDLK_v: {
                    if (SDL_GetModState() & KMOD_SHIFT) {
                      current_mode = VISUAL_LINE;
                      evil_visual_line(&editor);
                    } else {
                      current_mode = VISUAL;
                      evil_visual_char(&editor);
                    }
                  } break;
                      
                  case SDLK_4: {
                    if (SDL_GetModState() & KMOD_SHIFT) {
                      editor_move_to_line_end(&editor);
                    }
                  } break;
                      
                  case SDLK_a:
                    editor.last_stroke = SDL_GetTicks();
                    if (SDL_GetModState() & KMOD_CTRL) {
                      automatic_zoom = !automatic_zoom;
                      break;
                    } else if (SDL_GetModState() & KMOD_SHIFT) {
                      editor_move_to_line_end(&editor);
                    } else {
                        editor_move_char_right(&editor);
                    }

                    current_mode = INSERT;

                    // Eat up the next SDL_TEXTINPUT event for 'a' or 'A'
                    SDL_PollEvent(&tmpEvent); // This will typically be the SDL_TEXTINPUT event for 'a' or 'A'
                    if (tmpEvent.type != SDL_TEXTINPUT || (tmpEvent.text.text[0] != 'a' && tmpEvent.text.text[0] != 'A')) {
                      SDL_PushEvent(&tmpEvent); // If it's not, push it back to the event queue
                    }
                    break;

                    case SDLK_x:
                        if (editor.selection) {
                            editor_clipboard_copy(&editor);
                            editor_delete_selection(&editor);
                            editor.selection = false;
                        } else if (event.key.keysym.mod & KMOD_ALT) {
                            if (!M_x_active) {
                                current_mode = MINIBUFFER;
                                M_x_active = true;
                                editor.minibuffer_active = true;

                                // Consume the next SDL_TEXTINPUT event for 'x'
                                SDL_Event tmpEvent;
                                SDL_PollEvent(&tmpEvent);
                                if (tmpEvent.type != SDL_TEXTINPUT || tmpEvent.text.text[0] != 'x') {
                                    SDL_PushEvent(&tmpEvent); // Push the event back if it's not the one we're trying to consume
                                }
                            }
                            
                            // TODO fzy
                            /* if (!fzy) { */
                            /*     minibufferHeight += 189; */
                            /*     fzy = true; */
                            /* } */
                        } else if (event.key.keysym.mod & KMOD_SHIFT) {
                            evil_delete_backward_char(&editor);
                        } else if (event.key.keysym.mod & KMOD_CTRL) {
                            ctrl_x_pressed = true;
                        } else {
                            editor_clipboard_copy(&editor);
                            evil_delete_char(&editor);
                        }
                        break;

                  case SDLK_0:
                    editor_move_to_line_begin(&editor);
                    break;

                  case SDLK_F3:
                    file_browser = true;
                    break;

                  case SDLK_r:
                    if (event.key.keysym.mod & KMOD_CTRL) {
                        file_browser = true;
                    }
                    break;

                    case SDLK_BACKSPACE: //  yes you can delete in normal mode
                        if (editor.selection) {
                            editor_clipboard_copy(&editor);
                            editor_delete_selection(&editor);
                            editor.selection = false;
                        } else if (event.key.keysym.mod & KMOD_CTRL) {
                            emacs_backward_kill_word(&editor);
                        } else {
                            emacs_ungry_delete_backwards(&editor);
                            /* editor_backspace(&editor); */
                        }
                        break;
                    
                        
                    case SDLK_j:
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                        if ((event.key.keysym.mod & KMOD_CTRL) && (event.key.keysym.mod & KMOD_ALT)) {
                            evil_open_above(&editor);
                        } else if (event.key.keysym.mod & KMOD_CTRL) {
                            if (ctrl_x_pressed) {
                                file_browser = true;
                                ctrl_x_pressed = false;
                            } else {
                                editor_enter(&editor);
                            }
                        } else if ((event.key.keysym.mod & KMOD_ALT) && !followCursor) {
                            move_camera(&sr, "down", 50.0f);
                        } else if ((event.key.keysym.mod & KMOD_SHIFT) && !(event.key.keysym.mod & KMOD_ALT)) {
                            evil_join(&editor);
                        } else if (event.key.keysym.mod & KMOD_ALT) {
                            editor_drag_line_down(&editor);
                        } else {
                            editor_move_line_down(&editor);
                        }
                        mixSelectionColor = false;
                        editor.last_stroke = SDL_GetTicks();
                        break;


                    
                  case SDLK_k:
                    editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                    if ((event.key.keysym.mod & KMOD_ALT) && !followCursor) {
                      move_camera(&sr, "up", 50.0f);
                    } else if (event.key.keysym.mod & KMOD_CTRL) {
                      emacs_kill_line(&editor);
                    } else if (event.key.keysym.mod & KMOD_ALT) {
                      editor_drag_line_up(&editor);
                    } else if (event.key.keysym.mod & KMOD_SHIFT) {
                        goto_definition(&editor, &fb);
                    } else {
                        editor_move_line_up(&editor);
                    }
                    mixSelectionColor = false;
                    editor.last_stroke = SDL_GetTicks();
                    break;


                    case SDLK_h:
                        if (event.key.keysym.mod & KMOD_ALT) {
                            // If Alt is held, check if char under cursor is { or } and not editor->selection
                            emacs_mark_paragraph(&editor);
                        } else {
                            editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                            if (event.key.keysym.mod & KMOD_CTRL) {
                                editor_backspace(&editor);
                            } else {
                                editor_move_char_left(&editor);
                            }
                            // Toggle mixSelectionColor when Shift is pressed without Ctrl or Alt
                            mixSelectionColor = (event.key.keysym.mod & KMOD_SHIFT) && !(event.key.keysym.mod & KMOD_CTRL) && !(event.key.keysym.mod & KMOD_ALT);
                        }
                        editor.last_stroke = SDL_GetTicks();
                        break;

                    case SDLK_l:
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            showLineNumbers = !showLineNumbers;
                        } else if (event.key.keysym.mod & KMOD_ALT) {
                            select_region_from_inside_braces(&editor);
                        } else {
                            editor_move_char_right(&editor);
                            // Toggle mixSelectionColor when Shift is pressed without Ctrl or Alt
                            mixSelectionColor = (event.key.keysym.mod & KMOD_SHIFT) && !(event.key.keysym.mod & KMOD_CTRL) && !(event.key.keysym.mod & KMOD_ALT);
                        }
                        editor.last_stroke = SDL_GetTicks();
                        break;


                    
                    case SDLK_DOWN:
                        if (event.key.keysym.mod & KMOD_ALT) {
                            editor_drag_line_down(&editor);
                        } else {
                            editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                            if (event.key.keysym.mod & KMOD_CTRL) {
                                editor_move_paragraph_down(&editor);
                            } else {
                                editor_move_line_down(&editor);
                            }
                        }
                        break;
                        

                    case SDLK_UP:
                        if (event.key.keysym.mod & KMOD_ALT) {
                            editor_drag_line_up(&editor);
                        } else {
                            editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                            editor_move_line_up(&editor);

                            if (event.key.keysym.mod & KMOD_CTRL) {
                                editor_move_paragraph_up(&editor);
                            }
                        }
                        break;
    


                    case SDLK_RIGHT:
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                        editor_move_char_right(&editor);
                        break;

                    case SDLK_LEFT:
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                        editor_move_char_left(&editor);
                        break;


                  case SDLK_w:
                    if (event.key.keysym.mod & KMOD_CTRL) {
                      showWhitespaces = !showWhitespaces;
                    }else{
                      editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                      editor_move_word_right(&editor);
                    }
                    break;

                  case SDLK_e:
                    if (event.key.keysym.mod & KMOD_CTRL) {
                      isWave = !isWave;
                      /* current_mode = EMACS; */
                      /* editor.last_stroke = SDL_GetTicks(); */
                    }
                    break;

                    // additional NORMAL mode keybinds here...
                  } break;

                case INSERT:
                  switch (event.key.keysym.sym) {
                      SDL_Event tmpEvent;

                  case SDLK_x:
                    if (editor.selection) {
                      editor_clipboard_copy(&editor);
                      editor_delete_selection(&editor);
                      editor.selection = false;
                    }
                    break;


                    case SDLK_n: {
                        if (SDL_GetModState() & KMOD_CTRL) {
                            evil_complete_next(&editor);
                            editor.last_stroke = SDL_GetTicks();
                        }                      
                    }
                    break;

                    
                    case SDLK_SPACE: {
                        if (SDL_GetModState() & KMOD_CTRL) {
                            if (!editor.has_anchor){
                                editor_set_anchor(&editor);
                            } else {
                                editor_goto_anchor_and_clear(&editor);
                            }
                        }                      
                    }
                    break;
                    
                  case SDLK_i:
                      if (SDL_GetModState() & KMOD_ALT) {
                          if (SDL_GetModState() & KMOD_SHIFT) {
                              remove_one_indentation(&editor);
                          } else {
                              add_one_indentation(&editor);
                          }

                          editor.last_stroke = SDL_GetTicks();
                          // Eat up the next SDL_TEXTINPUT event for 'i' or 'I'
                          SDL_PollEvent(&tmpEvent);
                          if (tmpEvent.type != SDL_TEXTINPUT ||
                              (tmpEvent.text.text[0] != 'i' && tmpEvent.text.text[0] != 'I')) {
                              SDL_PushEvent(&tmpEvent); // Push it back to the event queue if it's not
                          }
                      }
                      break;
                      
                  case SDLK_o:
                      if (SDL_GetModState() & KMOD_CTRL) {
                          evil_open_below(&editor);
                          // Eat up the next SDL_TEXTINPUT event for 'o'
                          SDL_PollEvent(&tmpEvent);
                          if (tmpEvent.type != SDL_TEXTINPUT ||
                              (tmpEvent.text.text[0] != '0')) {
                              SDL_PushEvent(&tmpEvent); // Push it back to the event queue if it's not
                          }
                      }                          
                      editor.last_stroke = SDL_GetTicks();
                      break;
                      
                    case SDLK_a: {
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            editor.selection = true;
                            editor.select_begin = 0;
                            editor.cursor = editor.data.count;
                        }
                    }
                    break;
                    
                  case SDLK_h:
                      if (event.key.keysym.mod & KMOD_CTRL) {
                          editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                          editor_move_char_left(&editor);
                      } else if (event.key.keysym.mod & KMOD_ALT) {
                          editor_backspace(&editor);

                          // Eat up the next SDL_TEXTINPUT event for 'h'
                          SDL_PollEvent(&tmpEvent);
                          if (tmpEvent.type != SDL_TEXTINPUT ||
                              (tmpEvent.text.text[0] != 'h')) {
                              SDL_PushEvent(&tmpEvent); // Push it back to the event queue if it's not
                          }
                      }                          
                      editor.last_stroke = SDL_GetTicks();
                      break;
                      
                  case SDLK_j:
                      if (event.key.keysym.mod & KMOD_CTRL) {
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                        editor_move_line_down(&editor);
                      }
                      editor.last_stroke = SDL_GetTicks();
                      break;
                      
                  case SDLK_k:
                      if (event.key.keysym.mod & KMOD_CTRL) {
                          editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                          editor_move_line_up(&editor);
                      }
                      editor.last_stroke = SDL_GetTicks();
                      break;
                      
                  case SDLK_l:
                      if (event.key.keysym.mod & KMOD_CTRL) {
                          editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                          editor_move_char_right(&editor);
                      }
                      editor.last_stroke = SDL_GetTicks();
                      break;

                  case SDLK_TAB: {
                      activate_snippet(&editor);
                      break;
                  }

                  case SDLK_F3:
                      file_browser = true;
                      break;

                  case SDLK_MINUS:
                    if (SDL_GetModState() & KMOD_CTRL) {
                      zoom_factor += 1.0f;


                      if (zoom_factor > max_zoom_factor) {
                        zoom_factor = max_zoom_factor;
                      }

                      printf("zoom_factor = %f\n", zoom_factor);
                      // Consume the next SDL_TEXTINPUT event for '-'
                      SDL_Event tmpEvent;
                      SDL_PollEvent(&tmpEvent);
                      if (!(tmpEvent.type == SDL_TEXTINPUT && tmpEvent.text.text[0] == '-')) {
                        SDL_PushEvent(&tmpEvent);  // Push the event back if it's not the one we're trying to consume
                      }
                    }
                    break;

                  case SDLK_EQUALS:
                    if (SDL_GetModState() & KMOD_CTRL) {
                      zoom_factor -= 1.0f;

                      printf("zoom_factor = %f\n", zoom_factor);

                      if (zoom_factor < min_zoom_factor) {
                        zoom_factor = min_zoom_factor;
                      }

                      // Consume the next SDL_TEXTINPUT event for '='
                      SDL_Event tmpEvent;
                      SDL_PollEvent(&tmpEvent);
                      if (!(tmpEvent.type == SDL_TEXTINPUT && tmpEvent.text.text[0] == '=')) {
                        SDL_PushEvent(&tmpEvent);  // Push the event back if it's not the one we're trying to consume
                      }
                    }
                    break;

                  case SDLK_9: {
                    if (event.key.keysym.mod & KMOD_SHIFT) {
                      char pair[] = "()";
                      editor_insert_buf(&editor, pair, 2);
                      editor_move_char_left(&editor);

                      // Consume both characters '(' and ')' immediately
                      SDL_Event tmpEvent;
                      SDL_PollEvent(&tmpEvent);  // Consume '('
                      if (tmpEvent.type != SDL_TEXTINPUT || tmpEvent.text.text[0] != '(') {
                        SDL_PushEvent(&tmpEvent);
                      }
                      SDL_PollEvent(&tmpEvent);  // Consume ')'
                      if (tmpEvent.type != SDL_TEXTINPUT || tmpEvent.text.text[0] != ')') {
                        SDL_PushEvent(&tmpEvent);
                      }
                    } else {
                      editor_insert_char(&editor, '9');

                      // Consume the next SDL_TEXTINPUT event for '9'
                      SDL_Event tmpEvent;
                      SDL_PollEvent(&tmpEvent);
                      if (tmpEvent.type != SDL_TEXTINPUT || tmpEvent.text.text[0] != '9') {
                        SDL_PushEvent(&tmpEvent);
                      }
                    }
                  }
                    break;

                  case SDLK_LEFTBRACKET: {
                    if (event.key.keysym.mod & KMOD_SHIFT) {
                      char pair[] = "{}";
                      editor_insert_buf(&editor, pair, 2);
                      editor_move_char_left(&editor);

                      // Consume both characters '{' and '}' immediately
                      SDL_Event tmpEvent;
                      SDL_PollEvent(&tmpEvent);  // Consume '{'
                      if (tmpEvent.type != SDL_TEXTINPUT || tmpEvent.text.text[0] != '{') {
                        SDL_PushEvent(&tmpEvent);
                      }
                      SDL_PollEvent(&tmpEvent);  // Consume '}'
                      if (tmpEvent.type != SDL_TEXTINPUT || tmpEvent.text.text[0] != '}') {
                        SDL_PushEvent(&tmpEvent);
                      }
                    } else {
                      // Insert two '[' characters, move the cursor left, and consume the keypress
                      char pair[] = "[]";
                      editor_insert_buf(&editor, pair, 2);
                      editor_move_char_left(&editor);
                      SDL_Event tmpEvent;
                      SDL_PollEvent(&tmpEvent); // Consume '['
                      if (tmpEvent.type != SDL_TEXTINPUT || tmpEvent.text.text[0] != '[') {
                        SDL_PushEvent(&tmpEvent); // Push the event back if it's not the one we're trying to consume
                      }
                    }
                  }
                    break;

                  case SDLK_QUOTE: {
                    if (event.key.keysym.mod & KMOD_SHIFT) {
                      // If Shift + ' is pressed, insert double quotes ""
                      char pair[] = "\"\"";
                      editor_insert_buf(&editor, pair, 2);
                      editor_move_char_left(&editor);

                      // Consume both characters '"' and '"' immediately
                      SDL_Event tmpEvent;
                      SDL_PollEvent(&tmpEvent);  // Consume first '"'
                      if (tmpEvent.type != SDL_TEXTINPUT || tmpEvent.text.text[0] != '\"') {
                        SDL_PushEvent(&tmpEvent);
                      }
                      SDL_PollEvent(&tmpEvent);  // Consume second '"'
                      if (tmpEvent.type != SDL_TEXTINPUT || tmpEvent.text.text[0] != '\"') {
                        SDL_PushEvent(&tmpEvent);
                      }
                    } else {
                      // If just ' is pressed, insert single quotes ''
                      char pair[] = "''";
                      editor_insert_buf(&editor, pair, 2);
                      editor_move_char_left(&editor);
                      SDL_Event tmpEvent;
                      SDL_PollEvent(&tmpEvent); // Consume first '''
                      if (tmpEvent.type != SDL_TEXTINPUT || tmpEvent.text.text[0] != '\'') {
                        SDL_PushEvent(&tmpEvent); // Push the event back if it's not the one we're trying to consume
                      }
                    }
                    editor.last_stroke = SDL_GetTicks();
                  }
                    break;

                  case SDLK_BACKSPACE:
                      if (editor.selection) {
                          editor_clipboard_copy(&editor);
                          editor_delete_selection(&editor);
                          editor.selection = false;
                      } else if (event.key.keysym.mod & KMOD_CTRL) {
                          emacs_backward_kill_word(&editor);
                          editor.last_stroke = SDL_GetTicks();
                      }else if (event.key.keysym.mod & KMOD_ALT) {
                          emacs_ungry_delete_backwards(&editor);
                      } else {
                          editor_backspace(&editor);
                      }
                      editor.last_stroke = SDL_GetTicks();
                      break;

                  case SDLK_RETURN:
                      editor_enter(&editor);
                  break;
                    
                    case SDLK_f: {
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            editor_start_search(&editor);
                        }
                    }
                    break;

                    case SDLK_ESCAPE: {
                        if (editor.searching) {
                            editor_clear_mark(&editor);
                            editor_stop_search(&editor);
                        } else if (editor.minibuffer_active) {
                            editor.minibuffer_text.count = 0;
                            M_x_active = false;
                            editor.minibuffer_active = false;
                        }

                        current_mode = NORMAL;
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                    }
                    break;

                  case SDLK_c:
                    if (event.key.keysym.mod & KMOD_CTRL) {
                      editor_clipboard_copy(&editor);
                    }
                    break;

                  case SDLK_s: {
                    if (SDL_GetModState() & KMOD_CTRL) {  // Checks if CTRL is held down
                      if (editor.file_path.count > 0) {
                        err = editor_save(&editor);
                        if (err != 0) {
                          flash_error("Could not save currently edited file: %s", strerror(err));
                        }
                      } else {
                        // TODO: ask the user for the path to save to in this situation
                        flash_error("Nowhere to save the text");
                      }
                    }
                  }
                    break;

                    case SDLK_F5: {
                        simple_renderer_reload_shaders(&sr);
                    }
                    break;

                  case SDLK_v:
                    if (event.key.keysym.mod & KMOD_CTRL) {
                      editor_clipboard_paste(&editor);
                    }
                    break;

                  case SDLK_UP:
                    editor_update_selection(&editor,
                                            event.key.keysym.mod & KMOD_SHIFT);
                    if (event.key.keysym.mod & KMOD_CTRL) {
                      editor_move_paragraph_up(&editor);
                    } else {
                      editor_move_line_up(&editor);
                    }
                    editor.last_stroke = SDL_GetTicks();
                    break;

                  case SDLK_DOWN:
                    editor_update_selection(&editor,
                                            event.key.keysym.mod & KMOD_SHIFT);
                    if (event.key.keysym.mod & KMOD_CTRL) {
                      editor_move_paragraph_down(&editor);
                    } else {
                      editor_move_line_down(&editor);
                    }
                    editor.last_stroke = SDL_GetTicks();
                    break;

                  case SDLK_LEFT:
                    editor_update_selection(&editor,
                                            event.key.keysym.mod & KMOD_SHIFT);
                    if (event.key.keysym.mod & KMOD_CTRL) {
                      editor_move_word_left(&editor);
                    } else {
                      editor_move_char_left(&editor);
                    }
                    editor.last_stroke = SDL_GetTicks();
                    break;

                  case SDLK_RIGHT:
                    editor_update_selection(&editor,
                                            event.key.keysym.mod & KMOD_SHIFT);
                    if (event.key.keysym.mod & KMOD_CTRL) {
                      editor_move_word_right(&editor);
                    } else {
                      editor_move_char_right(&editor);
                    }
                    editor.last_stroke = SDL_GetTicks();
                    break;
                  }
                  break;


                case VISUAL:
                  switch (event.key.keysym.sym) {

                  case SDLK_y:
                    if (editor.selection) {
                      editor_clipboard_copy(&editor);
                    }
                    break;

                  case SDLK_x:
                    if (editor.selection) {
                      editor_clipboard_copy(&editor);
                      editor_delete_selection(&editor);
                      editor.selection = false;
                      current_mode = NORMAL;

                    }
                    break;
                    
                  case SDLK_j:
                    editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                    if (event.key.keysym.mod & KMOD_CTRL) {
                      editor_move_paragraph_down(&editor);
                    } else {
                      editor_move_line_down(&editor);
                    }
                    editor.last_stroke = SDL_GetTicks();
                    break;
                    
                  case SDLK_h:
                    editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                    if (event.key.keysym.mod & KMOD_CTRL) {
                      editor_move_word_left(&editor);
                    } else {
                      editor_move_char_left(&editor);
                    }
                    editor.last_stroke = SDL_GetTicks();
                    break;

                  case SDLK_k:
                    editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                    if (event.key.keysym.mod & KMOD_CTRL) {
                      editor_move_paragraph_up(&editor);
                    } else {
                      editor_move_line_up(&editor);
                    }
                    editor.last_stroke = SDL_GetTicks();
                    break;

                  case SDLK_l:
                    editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                    if (event.key.keysym.mod & KMOD_CTRL) {
                      editor_move_word_right(&editor);
                    } else {
                      editor_move_char_right(&editor);
                    }
                    editor.last_stroke = SDL_GetTicks();
                    break;

                  case SDLK_ESCAPE:
                    editor.selection = false;
                    current_mode = NORMAL;
                    break;
                  }
                  break;

                  // additional VISUAL mode keybinds here...

            case VISUAL_LINE:
              switch (event.key.keysym.sym) {

              case SDLK_j:
                  editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                  if (event.key.keysym.mod & KMOD_CTRL) {
                      editor_move_paragraph_down(&editor);
                  } else {
                      editor_move_line_down(&editor);

                      size_t cursor_row = editor_cursor_row(&editor);
                      if (cursor_row < editor.lines.count) {
                          // Set cursor to the end of this new line
                          Line current_line = editor.lines.items[cursor_row];
                          editor.cursor = current_line.end;
                      }
                  }
                  editor.last_stroke = SDL_GetTicks();
                  break;

              case SDLK_h:
                editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                if (event.key.keysym.mod & KMOD_CTRL) {
                  editor_move_word_left(&editor);
                } else {
                  editor_move_char_left(&editor);
                }
                editor.last_stroke = SDL_GetTicks();
                break;

              case SDLK_k:  // Up
                editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                if (event.key.keysym.mod & KMOD_CTRL) {
                  editor_move_paragraph_up(&editor);
                } else {
                  editor_move_line_up(&editor);
                  size_t cursor_row = editor_cursor_row(&editor);
                  if (cursor_row < editor.lines.count) {
                      // Set cursor to the end of this new line
                      Line current_line = editor.lines.items[cursor_row];
                      editor.cursor = current_line.begin;
                  }
                }
                editor.last_stroke = SDL_GetTicks();
                break;

              case SDLK_l:  // Right
                editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                if (event.key.keysym.mod & KMOD_CTRL) {
                  editor_move_word_right(&editor);
                } else {
                  editor_move_char_right(&editor);
                }
                editor.last_stroke = SDL_GetTicks();
                break;

                // Transition back to NORMAL mode
              case SDLK_ESCAPE:
                current_mode = NORMAL;
                break;

                // Add additional VISUAL_LINE mode keybinds here...
              }
              break;

             // TODO 
                case HELIX:
                    switch (event.key.keysym.sym) {
                    SDL_Event tmpEvent; // Declare once at the beginning of the switch block

                    case SDLK_RETURN: {
                        if (!toggle_bool(&editor)) {
                            editor_open_include(&editor);
                        }
                    } break;

                    case SDLK_SEMICOLON:
                        if (event.key.keysym.mod & KMOD_SHIFT) {
                            current_mode = MINIBUFFER;
                            evil_command_active = true;
                            editor.minibuffer_active = true;
                            
                            // Consume the next SDL_TEXTINPUT event for ':'
                            SDL_Event tmpEvent;
                            SDL_PollEvent(&tmpEvent);
                            if (tmpEvent.type != SDL_TEXTINPUT || tmpEvent.text.text[0] != ':') {
                                SDL_PushEvent(&tmpEvent); // Push the event back if it's not the one we're trying to consume
                            }
                            
                            // TODO fzy
                            /* if (!fzy) { */
                            /*     minibufferHeight += 189; */
                            /*     fzy = true; */
                            /* } */
                        }
                        break;


                        
                    case SDLK_d:
                        if (event.key.keysym.mod & KMOD_SHIFT) {
                            emacs_kill_line(&editor);
                        } else if (editor.selection) {
                            editor_clipboard_copy(&editor);
                            editor_delete_selection(&editor);
                            editor.selection = false;
                        } else {
                            emacs_kill_line(&editor);
                        }
                    break;
                                         
                    case SDLK_c:
                        if (event.key.keysym.mod & KMOD_SHIFT) {
                            evil_change_line(&editor);
                        }
                        
                        // Eat up the next SDL_TEXTINPUT event for 'C'
                        SDL_PollEvent(&tmpEvent);
                        if (tmpEvent.type != SDL_TEXTINPUT ||
                            (tmpEvent.text.text[0] != 'C')) {
                            SDL_PushEvent(&tmpEvent); // Push it back to the event queue if it's not
                        }
                        break;

                    case SDLK_m:
                        if (event.key.keysym.mod & KMOD_ALT) {
                            emacs_back_to_indentation(&editor);
                        }
                        break;



                    case SDLK_ESCAPE: {
                        if (fzy) {
                            minibufferHeight -= 189;
                            fzy = false;
                        }

                        if (editor.minibuffer_active) {
                            M_x_active = false;
                            editor.minibuffer_active = false;
                        }

                        editor_clear_mark(&editor);
                        editor_stop_search(&editor);
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                    }
                    break;


                    case SDLK_SPACE: {
                        if (SDL_GetModState() & KMOD_CTRL) {
                            if (!editor.has_anchor){
                                editor_set_anchor(&editor);
                            } else {
                                editor_goto_anchor_and_clear(&editor);
                            }
                        } else if (!fzy) {
                            // TODO time delay whichkey
                            minibufferHeight += 189;
                            fzy = true;
                        }
                    }
                    break;

                    case SDLK_5: {
                        if (SDL_GetModState() & KMOD_SHIFT) {
                            evil_jump_item(&editor);
                        }
                    }
                    break;

                    case SDLK_8: {
                        if (SDL_GetModState() & KMOD_SHIFT) {
                            evil_search_word_forward(&editor);
                        }
                    }
                    break;


                    case SDLK_1: {
                        if (SDL_GetModState() & KMOD_CTRL) {
                            hl_line = !hl_line;
                        }
                    }
                    break;


                    case SDLK_o:
                        if (SDL_GetModState() & KMOD_SHIFT) {
                            evil_open_above(&editor);
                        } else {
                            evil_open_below(&editor);
                        }
                        
                        current_mode = INSERT;
                        editor.last_stroke = SDL_GetTicks();
                        
                        // Eat up the next SDL_TEXTINPUT event for 'o' or 'O'
                        SDL_PollEvent(&tmpEvent);
                        if (tmpEvent.type != SDL_TEXTINPUT ||
                            (tmpEvent.text.text[0] != 'o' && tmpEvent.text.text[0] != 'O')) {
                            SDL_PushEvent(&tmpEvent); // Push it back to the event queue if it's not
                        }
                        break;

                  case SDLK_LEFTBRACKET:
                    if (SDL_GetModState() & KMOD_ALT) {
                      switch_to_font(library, &face, &atlas, -1);
                      printf("Switched to previous font: %s\n", fonts[current_font_index]);
                      /* redraw_screen(); */
                    }
                    break;

                  case SDLK_RIGHTBRACKET:
                    if (SDL_GetModState() & KMOD_ALT) {
                      switch_to_font(library, &face, &atlas, 1);
                      printf("Switched to next font: %s\n", fonts[current_font_index]);
                      /* redraw_screen(); */
                    }
                    break;


                    case SDLK_TAB: {
                        indent(&editor);
                    }
                    break;

                    case SDLK_z: {
                        if (SDL_GetModState() & KMOD_CTRL) {
                            /* current_mode = EMACS; */
                            /* current_mode = NORMAL; */
                            helix_mode();
                        }
                    }
                    break;

                  case SDLK_t: {
                    if (SDL_GetModState() & KMOD_CTRL) {
                      followCursor = !followCursor;  // Toggle the state
                    }
                  }
                    break;


                    case SDLK_F5: {
                        simple_renderer_reload_shaders(&sr);
                    }
                    break;

                    case SDLK_y:
                        if (editor.selection) {
                            editor_clipboard_copy(&editor);
                        } else {
                            evil_yank_line(&editor);
                        }
                        break;

                    
                  case SDLK_g: {
                    if (SDL_GetModState() & KMOD_SHIFT) {
                      editor_move_to_end(&editor);
                    } else {
                      editor_move_to_begin(&editor);
                    }
                  } break;

                  case SDLK_SLASH: {
                    current_mode = MINIBUFFER;
                    editor.last_stroke = SDL_GetTicks();
                    editor_start_search(&editor);

                    // Consume the next SDL_TEXTINPUT event for '/'
                    SDL_Event tmpEvent;
                    SDL_PollEvent(&tmpEvent);
                    if (tmpEvent.type != SDL_TEXTINPUT || tmpEvent.text.text[0] != '/') {
                      SDL_PushEvent(&tmpEvent); // Push the event back if it's not the one we're trying to consume
                    }
                  } break;

                    case SDLK_n: {
                        if (SDL_GetModState() & KMOD_CTRL) {
                            editor_move_line_down(&editor);
                        } else if (SDL_GetModState() & KMOD_ALT) {
                            editor_next_buffer(&editor);
                        } else if (SDL_GetModState() & KMOD_SHIFT) {
                            evil_search_previous(&editor);
                        } else {
                            evil_search_next(&editor);
                        }
                    } break;
                        
                    case SDLK_p:
                        if (SDL_GetModState() & KMOD_CTRL){
                            editor_move_line_up(&editor);
                        } else if (SDL_GetModState() & KMOD_ALT) {
                            editor_previous_buffer(&editor);
                        } else if (copiedLine) {
                            if (SDL_GetModState() & KMOD_SHIFT) {
                                evil_paste_before(&editor);
                            } else {
                                evil_paste_after(&editor);
                            }
                        } else {
                            editor_clipboard_paste(&editor);
                        }
                        break;
                        
                  case SDLK_b:
                    editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                    if (SDL_GetModState() & KMOD_CTRL){
                      editor_move_char_left(&editor);
                    } else if (SDL_GetModState() & KMOD_ALT) {
                        editor_kill_buffer(&editor);
                    } else {
                        editor_move_word_left(&editor);
                    }
                    break;

                  case SDLK_f:
                    if (SDL_GetModState() & KMOD_CTRL){
                      editor_move_char_right(&editor);
                    }
                    break;

                    case SDLK_s: {
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            // Ctrl+S is pressed
                            editor_start_search(&editor);
                            current_mode = MINIBUFFER;
                        } else {
                            // Either S or Shift+S is pressed
                            if (event.key.keysym.mod & KMOD_SHIFT) {
                                evil_change_whole_line(&editor);
                            } else {
                                evil_substitute(&editor);
                            }
                            editor.selection = false;
                            // Eat up the next SDL_TEXTINPUT event for 's' or 'S'
                            SDL_PollEvent(&tmpEvent);
                            if (tmpEvent.type != SDL_TEXTINPUT ||
                                (tmpEvent.text.text[0] != 's' && tmpEvent.text.text[0] != 'S')) {
                                SDL_PushEvent(&tmpEvent); // Push it back to the event queue if it's not
                            }
                            editor.last_stroke = SDL_GetTicks();
                        }
                        break;
                    }
                        
                  case SDLK_EQUALS: {
                    if (SDL_GetModState() & KMOD_ALT) {  // Check if ALT is pressed
                      theme_next(&currentThemeIndex);
                      printf("Changed theme to %d\n", currentThemeIndex); // Logging the theme change for debugging
                    } else if (SDL_GetModState() & KMOD_CTRL) {  // Check if CTRL is pressed
                      zoom_factor -= 1.0f;
                      if (zoom_factor < min_zoom_factor) {
                        zoom_factor = min_zoom_factor;
                      }
                    }
                  } break;

                  case SDLK_MINUS: {
                    if (SDL_GetModState() & KMOD_ALT) {  // Check if ALT is pressed
                      theme_previous(&currentThemeIndex);
                      printf("Changed theme back to %d\n", currentThemeIndex); // Logging the theme change for debugging
                    } else if (SDL_GetModState() & KMOD_CTRL) {  // Check if CTRL is pressed
                      zoom_factor += 1.0f;
                      if (zoom_factor > max_zoom_factor) {
                        zoom_factor = max_zoom_factor;
                      }
                    }
                  } break;

                    case SDLK_i:
                        if (SDL_GetModState() & KMOD_CTRL) {
                            showIndentationLines = !showIndentationLines;
                        } else if (SDL_GetModState() & KMOD_ALT) {
                            if (SDL_GetModState() & KMOD_SHIFT) {
                                remove_one_indentation(&editor);
                            } else {
                                add_one_indentation(&editor);
                            }
                        } else {
                            if (SDL_GetModState() & KMOD_SHIFT) {
                                evil_insert_line(&editor);
                            } else {
                                current_mode = INSERT;
                            }

                            editor.last_stroke = SDL_GetTicks();
                            
                            // Eat up the next SDL_TEXTINPUT event for 'i' or 'I'
                            SDL_PollEvent(&tmpEvent);
                            if (tmpEvent.type != SDL_TEXTINPUT ||
                                (tmpEvent.text.text[0] != 'i' && tmpEvent.text.text[0] != 'I')) {
                                SDL_PushEvent(&tmpEvent); // Push it back to the event queue if it's not
                            }
                        }
                        break;
                                          
                  case SDLK_v: {
                    if (SDL_GetModState() & KMOD_SHIFT) {
                      current_mode = VISUAL_LINE;
                      evil_visual_line(&editor);
                    } else {
                      current_mode = VISUAL;
                      evil_visual_char(&editor);
                    }
                  } break;
                      
                  case SDLK_4: {
                    if (SDL_GetModState() & KMOD_SHIFT) {
                      editor_move_to_line_end(&editor);
                    }
                  } break;
                      
                  case SDLK_a:
                    editor.last_stroke = SDL_GetTicks();
                    if (SDL_GetModState() & KMOD_SHIFT) { // Check if shift is being held
                      editor_move_to_line_end(&editor);
                    } else {
                      // Move the cursor one position to the right
                      editor_move_char_right(&editor);
                    }

                    current_mode = INSERT;

                    // Eat up the next SDL_TEXTINPUT event for 'a' or 'A'
                    SDL_PollEvent(&tmpEvent); // This will typically be the SDL_TEXTINPUT event for 'a' or 'A'
                    if (tmpEvent.type != SDL_TEXTINPUT || (tmpEvent.text.text[0] != 'a' && tmpEvent.text.text[0] != 'A')) {
                      SDL_PushEvent(&tmpEvent); // If it's not, push it back to the event queue
                    }
                    break;

                    case SDLK_x:
                        if (editor.selection) {
                            editor_clipboard_copy(&editor);
                            editor_delete_selection(&editor);
                            editor.selection = false;
                        } else if (event.key.keysym.mod & KMOD_ALT) {
                            if (!M_x_active) {
                                current_mode = MINIBUFFER;
                                M_x_active = true;
                                editor.minibuffer_active = true;

                                // Consume the next SDL_TEXTINPUT event for 'x'
                                SDL_Event tmpEvent;
                                SDL_PollEvent(&tmpEvent);
                                if (tmpEvent.type != SDL_TEXTINPUT || tmpEvent.text.text[0] != 'x') {
                                    SDL_PushEvent(&tmpEvent); // Push the event back if it's not the one we're trying to consume
                                }
                            }
                            
                            // TODO fzy
                            /* if (!fzy) { */
                            /*     minibufferHeight += 189; */
                            /*     fzy = true; */
                            /* } */
                        } else if (event.key.keysym.mod & KMOD_SHIFT) {
                            evil_delete_backward_char(&editor);
                        } else {
                            editor_clipboard_copy(&editor);
                            evil_delete_char(&editor);
                        }
                        break;

                  case SDLK_0:
                    editor_move_to_line_begin(&editor);
                    break;

                  case SDLK_F3:
                    file_browser = true;
                    break;

                  case SDLK_r:
                    if (event.key.keysym.mod & KMOD_CTRL) {
                    file_browser = true;
                    }
                    break;

                  case SDLK_BACKSPACE: //  yes you can delete in normal mode
                    if (editor.selection) {
                      editor_clipboard_copy(&editor);
                      editor_delete_selection(&editor);
                      editor.selection = false;
                    } else if (event.key.keysym.mod & KMOD_CTRL) {
                      emacs_backward_kill_word(&editor);
                    } else {
                      editor_backspace(&editor);
                    }
                    break;
                    
                    case SDLK_h:
                        if (event.key.keysym.mod & KMOD_ALT) {
                            emacs_mark_paragraph(&editor);
                        } else {
                            editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                            if (event.key.keysym.mod & KMOD_CTRL) {
                                editor_move_word_left(&editor);
                            } else {
                                editor_move_char_left(&editor);
                            }
                        }
                        editor.last_stroke = SDL_GetTicks();
                        break;

                        
                  case SDLK_j:
                    editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                    if ((event.key.keysym.mod & KMOD_ALT) && !followCursor) {
                      move_camera(&sr, "down", 50.0f);
                    } else if (event.key.keysym.mod & KMOD_CTRL) {
                      evil_open_above(&editor);
                    } else if ((event.key.keysym.mod & KMOD_SHIFT) && !(event.key.keysym.mod & KMOD_ALT)) {
                      evil_join(&editor);
                    } else if (event.key.keysym.mod & KMOD_ALT) {
                      editor_move_paragraph_down(&editor);
                    } else {
                      editor_move_line_down(&editor);
                    }
                    editor.last_stroke = SDL_GetTicks();
                    break;

                  case SDLK_k:
                    editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                    if ((event.key.keysym.mod & KMOD_ALT) && !followCursor) {
                      move_camera(&sr, "up", 50.0f);
                    } else if (event.key.keysym.mod & KMOD_CTRL) {
                      emacs_kill_line(&editor);
                    } else if (event.key.keysym.mod & KMOD_ALT) {
                      editor_move_paragraph_up(&editor);
                    } else if (event.key.keysym.mod & KMOD_SHIFT) {
                        goto_definition(&editor, &fb);
                    } else {
                        editor_move_line_up(&editor);
                    }
                    editor.last_stroke = SDL_GetTicks();
                    break;

                    case SDLK_l:
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            showLineNumbers = !showLineNumbers;
                        } else if (event.key.keysym.mod & KMOD_ALT) {
                            select_region_from_inside_braces(&editor); 
                        } else {
                            editor_move_char_right(&editor);
                        }
                        editor.last_stroke = SDL_GetTicks();
                        break;

                    
                    case SDLK_DOWN:
                        if (event.key.keysym.mod & KMOD_ALT) {
                            editor_drag_line_down(&editor);
                        } else {
                            editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                            if (event.key.keysym.mod & KMOD_CTRL) {
                                editor_move_paragraph_down(&editor);
                            } else {
                                editor_move_line_down(&editor);
                            }
                        }
                        break;
                        

                    case SDLK_UP:
                        if (event.key.keysym.mod & KMOD_ALT) {
                            editor_drag_line_up(&editor);
                        } else {
                            editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                            editor_move_line_up(&editor);

                            if (event.key.keysym.mod & KMOD_CTRL) {
                                editor_move_paragraph_up(&editor);
                            }
                        }
                        break;
    


                    case SDLK_RIGHT:
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                        editor_move_char_right(&editor);
                        break;

                    case SDLK_LEFT:
                        editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                        editor_move_char_left(&editor);
                        break;


                  case SDLK_w:
                    if (event.key.keysym.mod & KMOD_CTRL) {
                      showWhitespaces = !showWhitespaces;
                    }else{
                      editor_update_selection(&editor, event.key.keysym.mod & KMOD_SHIFT);
                      editor_move_word_right(&editor);
                    }
                    break;

                  case SDLK_e:
                    if (event.key.keysym.mod & KMOD_CTRL) {
                      isWave = !isWave;
                      /* current_mode = EMACS; */
                      /* editor.last_stroke = SDL_GetTicks(); */
                    }
                    break;

                    // additional NORMAL mode keybinds here...
                  } break;
              
                case MINIBUFFER:
                    switch (event.key.keysym.sym) {

                    case SDLK_ESCAPE: {
                        if (fzy) {
                            minibufferHeight -= 189;
                            fzy = false;
                        }

                        if (editor.searching) {
                            editor_clear_mark(&editor);
                            editor_stop_search(&editor);
                        } else if (editor.minibuffer_active) {
                            editor.minibuffer_text.count = 0;
                            M_x_active = false;
                            editor.minibuffer_active = false;
                        }
                        current_mode = NORMAL;
                    }
                    break;
                
                    case SDLK_BACKSPACE:
                        if (editor.selection) {
                            // TODO once we have selection in the minibuffer
                            /* editor_clipboard_copy(&editor); */
                            /* editor_delete_selection(&editor); */
                            /* editor.selection = false; */
                        } else if (event.key.keysym.mod & KMOD_CTRL) {
                            emacs_backward_kill_word(&editor);
                            editor.last_stroke = SDL_GetTicks();
                        }else{
                            editor_backspace(&editor);
                        }
                        editor.last_stroke = SDL_GetTicks();
                        break;

                    case SDLK_RETURN: {
                        editor_enter(&editor);
                    }
                    break;
                }
                break;

              // More cases for other modes can follow here...
              // ...
                }
                break;
              }
              break;


            case SDL_TEXTINPUT:
              if (file_browser) {
                // Once we have incremental search in the file browser this may become useful
                // or to edit file names or create files/direcory
              } else if (current_mode == INSERT || current_mode == EMACS || current_mode == MINIBUFFER) { // Process text input

                if (editor.selection) {
                    editor_delete_selection(&editor);
                }

                const char *text = event.text.text;
                size_t text_len = strlen(text);
                for (size_t i = 0; i < text_len; ++i) {
                  editor_insert_char(&editor, text[i]);
                }

                editor.selection = false;
                editor.last_stroke = SDL_GetTicks();
              }
              reset_keychords();
              break;

            }
        }

        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        glViewport(0, 0, w, h);
        /* Vec4f bg = themes[currentThemeIndex].background; */
        Vec4f bg = currentTheme.background;
        bg.w = 0.0f;
        glClearColor(bg.x, bg.y, bg.z, bg.w);
        glClear(GL_COLOR_BUFFER_BIT);

        if (file_browser) {
          fb_render(&fb, window, &atlas, &sr);
        } else {
            if (theme_lerp){
                update_theme_interpolation();
            }
            update_modeline_animation();
            update_minibuffer_animation(DELTA_TIME);

            editor_render(window, &atlas, &sr, &editor);
            update_cursor_color(&editor);

            if (fb.file_extension.items != NULL && strcmp(fb.file_extension.items, "md") == 0) {
                render_markdown(&atlas, &sr, &editor, &fb);
            }

          
          if (M_x_active){
              render_minibuffer_content(&atlas, &sr, &editor, "M-x");
          } else if (evil_command_active) {
              render_minibuffer_content(&atlas, &sr, &editor, ":");
          } else if (editor.searching) {
              /* render_minibuffer_content(&atlas, &sr, &editor, "/"); */
              render_search_text(&atlas, &sr, &editor);
          }
        }

        SDL_GL_SwapWindow(window);
        const Uint32 duration = SDL_GetTicks() - start;
        const Uint32 delta_time_ms = 1000 / FPS;
        if (duration < delta_time_ms) {
            SDL_Delay(delta_time_ms - duration);
        }
    }

    shutdown_clangd(&editor);
    free_snippet_array(&snippets);
    return 0;
}

// TODO: ability to search within file browser
// Very useful when you have a lot of files
// TODO: ability to remove trailing whitespaces
