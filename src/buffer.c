#include "buffer.h"



// BUFFER
// TODO switching buffers delete unsaved changes
// TODO save cursor position on each buffer 

void editor_add_to_buffer_history(Editor *e, const char *file_path) {
    if (e->buffer_history_count < MAX_BUFFER_HISTORY) {
        free(e->buffer_history[e->buffer_history_count]); // Free existing string if any
        e->buffer_history[e->buffer_history_count] = strdup(file_path);
    }
    e->buffer_index = e->buffer_history_count; // Update buffer index
    e->buffer_history_count++;
}


void editor_remove_from_buffer_history(Editor *e) {
    if (e->buffer_history_count > 0) {
        free(e->buffer_history[--e->buffer_history_count]); // Free the last string
    }
}


Errno editor_open_buffer(Editor *e, const char *file_path) {
    printf("Opening buffer: %s\n", file_path);

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

void editor_kill_buffer(Editor *e) {
    if (e->buffer_history_count > 0) {
        // Free the current buffer path and remove it from the history
        free(e->buffer_history[e->buffer_index]);
        e->buffer_history[e->buffer_index] = NULL;

        // Shift all elements after the current index down
        for (int i = e->buffer_index; i < e->buffer_history_count - 1; i++) {
            e->buffer_history[i] = e->buffer_history[i + 1];
        }

        // Decrease the count of buffers in the history
        e->buffer_history_count--;

        // Update the buffer index to point to the previous buffer, if possible


        
        if (e->buffer_index > 0) {
        }

        // If there are still buffers in the history, load the previous one
        if (e->buffer_history_count > 0) {
            const char *prev_file_path = e->buffer_history[e->buffer_index];
            editor_open_buffer(e, prev_file_path); // Open the previous buffer without adding to history
        } else {
            // Handle the case when there are no more buffers in the history
            // For example open a scratch buffer
        }
    }
}


void editor_previous_buffer(Editor *e) {
    if (e->buffer_index > 0) {
        e->buffer_index--; // Move to the previous buffer in history
        const char *prev_file_path = e->buffer_history[e->buffer_index];
        editor_open_buffer(e, prev_file_path); // Open the previous buffer
    } else {
        // Handle case when there's no previous buffer
        printf("No previous buffer available.\n");
    }
}

void editor_next_buffer(Editor *e) {
    if (e->buffer_index < e->buffer_history_count - 1) {
        e->buffer_index++; // Move to the next buffer in history
        const char *next_file_path = e->buffer_history[e->buffer_index];
        editor_open_buffer(e, next_file_path); // Open the next buffer
    } else {
        // Handle case when there's no next buffer
        printf("No next buffer available.\n");
    }
}
