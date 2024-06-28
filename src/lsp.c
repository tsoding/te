#include "lsp.h"
#include "common.h"
#include "editor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <json-c/json.h>
#include <limits.h>
#include "common.h"

int to_clangd[2];
int from_clangd[2];
pthread_t receive_thread;
int current_request_id = 1;

void handle_error(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void start_clangd(Editor *e) {
    printf("Starting clangd...\n");
    if (pipe(to_clangd) == -1 || pipe(from_clangd) == -1) {
        handle_error("Failed to create pipes");
    }

    pid_t pid = fork();
    if (pid == -1) {
        handle_error("Failed to fork");
    } else if (pid == 0) { // Child process
        // Close unused pipe ends
        close(to_clangd[1]);
        close(from_clangd[0]);

        // Redirect stdin and stdout
        dup2(to_clangd[0], STDIN_FILENO);
        dup2(from_clangd[1], STDOUT_FILENO);

        execlp("clangd", "clangd", NULL);
        handle_error("Failed to start clangd");
    } else { // Parent process
        // Close unused pipe ends
        close(to_clangd[0]);
        close(from_clangd[1]);

        e->to_clangd_fd = to_clangd[1];
        e->from_clangd_fd = from_clangd[0];
        if (pthread_create(&receive_thread, NULL, receive_json_rpc, e) != 0) {
            handle_error("Failed to create thread for receive_json_rpc");
        }
    }
    send_initialize_request(e);
}

void shutdown_clangd(Editor *e) {
    send_json_rpc(e->to_clangd_fd, "shutdown", "{}", current_request_id++);
    send_json_rpc(e->to_clangd_fd, "exit", "{}", current_request_id++);
    close(e->to_clangd_fd);
    pthread_join(receive_thread, NULL);
    close(e->from_clangd_fd);
    wait(NULL); // Wait for clangd to terminate
}

void send_json_rpc(int fd, const char* method, const char* params, int request_id) {
    char message[4096];
    size_t message_length = snprintf(message, sizeof(message), "{\"jsonrpc\": \"2.0\", \"id\": %d, \"method\": \"%s\", \"params\": %s}\n",
                                  request_id, method, params);

    if (message_length >= sizeof(message)) {
        fprintf(stderr, "[send_json_rpc] Error: JSON-RPC message is too long.\n");
        return;
    }

    ssize_t bytes_written = write(fd, message, message_length);
    if (bytes_written == -1) {
        perror("[send_json_rpc] Error sending JSON-RPC");
    } else if ((size_t)bytes_written != message_length) {
        fprintf(stderr, "[send_json_rpc] Error: Partial JSON-RPC message sent. Expected %zu bytes, sent %zu bytes.\n", message_length, (size_t)bytes_written);
    } else {
        printf("[send_json_rpc] Sent %zu bytes: %s\n", (size_t)bytes_written, message);
    }
}

void parse_lsp_response(const char *response_json, LSPResponse *response) {
    json_object *parsed_json = json_tokener_parse(response_json);
    json_object *id, *method, *params;

    if (json_object_object_get_ex(parsed_json, "id", &id)) {
        response->id = json_object_get_int(id);
    }

    if (json_object_object_get_ex(parsed_json, "method", &method)) {
        response->method = strdup(json_object_get_string(method));
    }

    if (json_object_object_get_ex(parsed_json, "params", &params)) {
        response->params = strdup(json_object_to_json_string(params));
    }

    json_object_put(parsed_json);
}

void handle_lsp_response(LSPResponse *response, Editor *e) {
    printf("[handle_lsp_response] Received response with method: %s\n", response->method);

    if (strcmp(response->method, "textDocument/definition") == 0) {
        json_object *parsed_response = json_tokener_parse(response->params);
        json_object *locations_array;

        if (json_object_object_get_ex(parsed_response, "result", &locations_array)) {
            // Assuming the result is an array of locations (as per LSP specification)
            json_object *location = json_object_array_get_idx(locations_array, 0); // Get the first location
            if (location) {
                json_object *uri_obj, *range_obj, *start_obj, *line_obj, *char_obj;
                if (json_object_object_get_ex(location, "uri", &uri_obj) &&
                    json_object_object_get_ex(location, "range", &range_obj) &&
                    json_object_object_get_ex(range_obj, "start", &start_obj) &&
                    json_object_object_get_ex(start_obj, "line", &line_obj) &&
                    json_object_object_get_ex(start_obj, "character", &char_obj)) {

                    const char *file_uri = json_object_get_string(uri_obj);
                    int line = json_object_get_int(line_obj);
                    int character = json_object_get_int(char_obj);

                    char file_path[PATH_MAX];
                    convert_uri_to_file_path(file_uri, file_path, sizeof(file_path));
                    printf("[handle_lsp_response] Definition found at file: %s, line: %d, character: %d\n", file_path, line, character);
                    find_file(e, file_path, line, character);
                }
            }
        }
        json_object_put(parsed_response);
    } else {
        printf("[handle_lsp_response] Received non-definition response or method not recognized\n");
    }
}

void* receive_json_rpc(void* arg) {
    Editor *e = (Editor *)arg;
    if (e == NULL) {
        fprintf(stderr, "[receive_json_rpc] Editor instance is NULL\n");
        return NULL;
    }

    char buffer[4096];
    ssize_t nbytes;

    printf("[receive_json_rpc] Thread started, waiting for responses from clangd...\n");

    while (1) {
        nbytes = read(e->from_clangd_fd, buffer, sizeof(buffer) - 1);

        if (nbytes > 0) {
            buffer[nbytes] = '\0';
            printf("[receive_json_rpc] Received %zd bytes from clangd: %s\n", nbytes, buffer);

            LSPResponse response;
            parse_lsp_response(buffer, &response);

            if (response.method) {
                printf("[receive_json_rpc] Handling response for method: %s\n", response.method);
                handle_lsp_response(&response, e);
                free(response.method);
                free(response.params);
            } else {
                printf("[receive_json_rpc] No valid method found in response or response parsing failed\n");
            }
        } else if (nbytes == 0) {
            printf("[receive_json_rpc] EOF reached, clangd might have closed the connection.\n");
            break;
        } else {
            perror("[receive_json_rpc] Error reading from clangd");
            break;
        }
    }

    printf("[receive_json_rpc] Thread is exiting.\n");
    return NULL;
}


void convert_uri_to_file_path(const char *uri, char *file_path, size_t file_path_size) {
    if (strncmp(uri, "file://", 7) == 0) {
        uri += 7; // Skip the "file://" part
        char *decoded_uri = url_decode(uri); // Implement url_decode to handle percent-encoding
        snprintf(file_path, file_path_size, "%s", decoded_uri);
        free(decoded_uri); // Assuming url_decode dynamically allocates memory
    } else {
        fprintf(stderr, "Invalid URI format\n");
        strncpy(file_path, "", file_path_size);
    }
}

// Example implementation of url_decode (simplified)
char *url_decode(const char *str) {
    char *decoded = malloc(strlen(str) + 1);
    char *d = decoded;
    while (*str) {
        if (*str == '%' && *(str + 1) && *(str + 2)) {
            char hex[3] = { str[1], str[2], '\0' };
            *d++ = (char)strtol(hex, NULL, 16);
            str += 3;
        } else {
            *d++ = *str++;
        }
    }
    *d = '\0';
    return decoded;
}

void goto_definition(Editor *e, File_Browser *fb) {
    if (!e || !fb) {
        fprintf(stderr, "[goto_definition] Error: Editor or File_Browser is NULL\n");
        return;
    }

    char file_uri[256];
    get_current_file_uri(e, fb, file_uri, sizeof(file_uri));
    int character;
    size_t line;
    /* get_cursor_position(e, &line, &character); */
    get_cursor_position(e);

    char params[512];
    int params_length = snprintf(params, sizeof(params),
             "{\"textDocument\": {\"uri\": \"%s\"}, \"position\": {\"line\": %zu, \"character\": %d}}",
             file_uri, line, character);

    // Check for snprintf error
    if (params_length < 0) {
        fprintf(stderr, "[goto_definition] Error: Encoding error in snprintf.\n");
        return;
    }

    // Now safe to compare, with casting to match types
    if (params_length >= (int)sizeof(params)) {
        fprintf(stderr, "[goto_definition] Error: Params string is too long.\n");
        return;
    }

    send_json_rpc(e->to_clangd_fd, "textDocument/definition", params, current_request_id++);
    printf("[goto_definition] Requested definition at URI: %s, Line: %zu, Character: %d\n", file_uri, line, character);
}

void get_current_file_uri(Editor *e, File_Browser *fb, char *file_uri, size_t uri_size) {
    if (!e || !fb || !file_uri) {
        fprintf(stderr, "Error: Invalid arguments in get_current_file_uri\n");
        return;
    }

    // Directly access the items of the String_Builder
    /* char *path = fb->dir_path.items; */
    char *path = fb->file_path.items;
    /* char *path = "/home/l/Desktop/test/ded"; */

    if (path && path[0] != '\0') {  // Check if the path is not empty
        snprintf(file_uri, uri_size, "file://%s", path);
    } else {
        fprintf(stderr, "Error: File path is empty in File_Browser\n");
        strncpy(file_uri, "", uri_size);
    }
}

void send_initialize_request(Editor *e) {
    const char *params = "{"
                         "\"processId\": null,"
                         "\"rootUri\": \"file:///home/l/Desktop/test/ded\","
                         "\"capabilities\": {"
                         "  \"textDocument\": {"
                         "    \"definition\": {"
                         "      \"dynamicRegistration\": true"
                         "    }"
                         "  }"
                         "}"
                         "}";
    send_json_rpc(e->to_clangd_fd, "initialize", params, current_request_id++);
}



void send_initialized_notification(Editor *e) {
    send_json_rpc(e->to_clangd_fd, "initialized", "{}", current_request_id++);
}

void send_did_open_notification(Editor *e, const char *file_uri, const char *file_content) {
    char params[1024];
    snprintf(params, sizeof(params),
             "{\"textDocument\": {"
             "\"uri\": \"%s\","
             "\"languageId\": \"c\","
             "\"version\": 1,"
             "\"text\": \"%s\""
             "}}", file_uri, file_content);
    send_json_rpc(e->to_clangd_fd, "textDocument/didOpen", params, current_request_id++);
}


