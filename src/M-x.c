#include "M-x.h"
#include "editor.h"
#include "hashmap.h"
#include "evil.h"
#include "helix.h"
#include "emacs.h"
#include "lsp.h"
#include "utilities.h"

// TODO aliases (lua or lisp we will have an init file), 
// history in program memory, when quitting save it in ~/.config/ded/M-x-history
// and load it when opening ded clamp it to max-M-x-history-size or something


void register_command(struct hashmap *command_map, const char *name, void (*execute)(Editor *, const char *params[]), int additional_params_count) {
    Command *cmd = malloc(sizeof(Command));
    if (cmd) {
        cmd->name = name;
        cmd->execute = execute;
        cmd->additional_params_count = additional_params_count;
        hashmap_set(command_map, cmd);
    } else {
        // Handle allocation failure
    }
}

// TODO open-below && open-above && editor-enter behave weird
void initialize_commands(struct hashmap *command_map) {
    register_command(command_map, "open",            evil_open_below, 0);
    register_command(command_map, "opena",           evil_open_above, 0);
    register_command(command_map, "drag-down",       editor_drag_line_down, 0);
    register_command(command_map, "drag-up",         editor_drag_line_up, 0);
    register_command(command_map, "editor-enter",    editor_enter, 0);
    register_command(command_map, "select",          select_region_from_brace, 0);
    register_command(command_map, "back",            emacs_backward_kill_word, 0);
    register_command(command_map, "evil-join",       evil_join, 0);
    register_command(command_map, "evil-yank-line",  evil_yank_line, 0);
    register_command(command_map, "open-include",    editor_open_include, 0);
    register_command(command_map, "toggle",          toggle_bool, 0); // Wincompatible-function-pointer-types
    register_command(command_map, "w",               editor_save, 0);
    register_command(command_map, "q",               editor_quit, 0);
    register_command(command_map, "wq",              editor_save_and_quit, 0);
    register_command(command_map, "go",              editor_goto_line, 1);
    register_command(command_map, "def",             goto_definition, 0);
    register_command(command_map, "helix",           helix_mode, 0);
 
}

// TODO if you provide less arguments than needed warn  the cursor
// TODO if the function fail print it and maybe with the actuall error code or string
void execute_command(struct hashmap *command_map, Editor *editor, const char *input) {
    // First, check if the input is a number
    if (is_number(input)) {
        const char *params[2] = {input, NULL};
        editor_goto_line(editor, params);
        return;
    }

    char command_name[100];
    const char *params[10] = {0};
    int params_count = 0;

    // Duplicate the input string for safe tokenization
    char *input_copy = strdup(input);
    char *token = strtok(input_copy, " ");

    if (token != NULL) {
        strncpy(command_name, token, sizeof(command_name) - 1);
        command_name[sizeof(command_name) - 1] = '\0';

        // Extract arguments
        while ((token = strtok(NULL, " ")) != NULL && params_count < 10) {
            params[params_count++] = token;
        }
    }

    Command tempCmd = {command_name, NULL, 0};
    Command *cmd = (Command *)hashmap_get(command_map, &tempCmd);
    if (cmd) {
        if (cmd->additional_params_count == params_count) {
            cmd->execute(editor, params);
        } else if (cmd->additional_params_count == 0 && params_count == 0) {
            cmd->execute(editor, NULL);
        } else {
            // Handle incorrect number of arguments
        }
    } else {
        // Command not found
    }

    free(input_copy);
}

int command_compare(const void *a, const void *b, void *udata) {
    const Command *cmd_a = a;
    const Command *cmd_b = b;
    return strcmp(cmd_a->name, cmd_b->name);
}

uint64_t simple_string_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const Command *cmd = item;
    const char *str = cmd->name;
    uint64_t hash = seed0;
    while (*str) {
        hash = 31 * hash + (*str++);
    }
    return hash ^ seed1;
}


// HISTORY


/* #define MAX_HISTORY 500 */

/* typedef struct { */
/*     char **items; */
/*     size_t count; */
/*     size_t capacity; */
/* } History; */

/* History command_history = {NULL, 0, 0}; */

/* void add_to_history(const char *command) { */
/*     if (command_history.count >= MAX_HISTORY) { */
/*         // Remove the oldest command */
/*         free(command_history.items[0]); */
/*         memmove(command_history.items, command_history.items + 1, (MAX_HISTORY - 1) * sizeof(char*)); */
/*         command_history.count--; */
/*     } */

/*     if (command_history.count == command_history.capacity) { */
/*         // Increase the capacity */
/*         size_t new_capacity = command_history.capacity == 0 ? 16 : command_history.capacity * 2; */
/*         char **new_items = realloc(command_history.items, new_capacity * sizeof(char*)); */
/*         if (!new_items) { */
/*             // Handle allocation failure */
/*             return; */
/*         } */
/*         command_history.items = new_items; */
/*         command_history.capacity = new_capacity; */
/*     } */

/*     command_history.items[command_history.count++] = strdup(command); */
/* } */

/* // Call this function in execute_command after successfully executing a command */
/* add_to_history(input); */

