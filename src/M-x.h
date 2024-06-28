#ifndef META_H_
#define META_H_

#include <stdint.h> // for uint64_t
#include "editor.h"

typedef struct {
    const char *name;
    void (*execute)(Editor *, const char *params[]); // Updated to take an array of strings as additional parameters
    int additional_params_count; // Number of additional parameters needed
} Command;

void register_command(struct hashmap *command_map, const char *name, void (*execute)(Editor *, const char *params[]), int additional_params_count);
void initialize_commands(struct hashmap *command_map);
void execute_command(struct hashmap *command_map, Editor *editor, const char *command_name);
int command_compare(const void *a, const void *b, void *udata);
uint64_t simple_string_hash(const void *item, uint64_t seed0, uint64_t seed1);

#endif // META_H__
