#ifndef LSP_H_
#define LSP_H_

#include "file_browser.h"
#include "editor.h"
#include <pthread.h>

// Struct to store LSP response
typedef struct {
    int id;
    char *method;
    char *params;
} LSPResponse;

extern pthread_t receive_thread;


/* void start_clangd(); */
void start_clangd(Editor *e);
/* void shutdown_clangd(); */
void shutdown_clangd(Editor *e);
/* void send_json_rpc(const char* method, const char* params, int request_id); */
void send_json_rpc(int fd, const char* method, const char* params, int request_id);
void* receive_json_rpc(void* arg);
void goto_definition(Editor *e, File_Browser *fb);
void handle_lsp_response(LSPResponse *response, Editor *e);
void convert_uri_to_file_path(const char *uri, char *file_path, size_t file_path_size);
char *url_decode(const char *str);
void get_current_file_uri(Editor *e, File_Browser *fb, char *file_uri, size_t uri_size);
void parse_lsp_response(const char *response_json, LSPResponse *response);




void send_did_open_notification(Editor *e, const char *file_uri, const char *file_content);
void send_initialize_request(Editor *e);
void send_initialized_notification(Editor * e);

#endif // LSP_H_
