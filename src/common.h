#ifndef COMMON_H_
#define COMMON_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "./la.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define FPS 144
#define DELTA_TIME (1.0f / FPS)
#define CURSOR_OFFSET 0.13f

typedef int Errno;

#define SWAP(T, a, b) do { T t = a; a = b; b = t; } while (0)

#define return_defer(value) do { result = (value); goto defer; } while (0)

#define UNIMPLEMENTED(...)                                                      \
    do {                                                                        \
        printf("%s:%d: UNIMPLEMENTED: %s \n", __FILE__, __LINE__, __VA_ARGS__); \
        exit(1);                                                                \
    } while(0)
#define UNREACHABLE(...)                                                      \
    do {                                                                      \
        printf("%s:%d: UNREACHABLE: %s \n", __FILE__, __LINE__, __VA_ARGS__); \
        exit(1);                                                              \
    } while(0)
#define UNUSED(x) (void)(x)

#define DA_INIT_CAP 256

#define da_last(da) (assert((da)->count > 0), (da)->items[(da)->count - 1])

#define da_move(dst, src)                \
    do {                                 \
       free((dst)->items);               \
       (dst)->items = (src).items;       \
       (dst)->count = (src).count;       \
       (dst)->capacity = (src).capacity; \
    } while (0)

#define da_append(da, item)                                                          \
    do {                                                                             \
        if ((da)->count >= (da)->capacity) {                                         \
            (da)->capacity = (da)->capacity == 0 ? DA_INIT_CAP : (da)->capacity*2;   \
            (da)->items = realloc((da)->items, (da)->capacity*sizeof(*(da)->items)); \
            assert((da)->items != NULL && "Buy more RAM lol");                       \
        }                                                                            \
                                                                                     \
        (da)->items[(da)->count++] = (item);                                         \
    } while (0)

#define da_append_many(da, new_items, new_items_count)                                      \
    do {                                                                                    \
        if ((da)->count + new_items_count > (da)->capacity) {                               \
            if ((da)->capacity == 0) {                                                      \
                (da)->capacity = DA_INIT_CAP;                                               \
            }                                                                               \
            while ((da)->count + new_items_count > (da)->capacity) {                        \
                (da)->capacity *= 2;                                                        \
            }                                                                               \
            (da)->items = realloc((da)->items, (da)->capacity*sizeof(*(da)->items));        \
            assert((da)->items != NULL && "Buy more RAM lol");                              \
        }                                                                                   \
        memcpy((da)->items + (da)->count, new_items, new_items_count*sizeof(*(da)->items)); \
        (da)->count += new_items_count;                                                     \
    } while (0)

char *temp_strdup(const char *s);
void temp_reset(void);

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} String_Builder;

#define SB_Fmt "%.*s"
#define SB_Arg(sb) (int) (sb).count, (sb).items

#define sb_append_buf da_append_many
#define sb_append_cstr(sb, cstr)  \
    do {                          \
        const char *s = (cstr);   \
        size_t n = strlen(s);     \
        da_append_many(sb, s, n); \
    } while (0)
#define sb_append_null(sb) da_append_many(sb, "", 1)

#define sb_to_sv(sb) sv_from_parts((sb).items, (sb).count)

// ADDED
typedef struct {
    char *name;
    char *permissions;
    long size;
    char *mod_time;
    char *owner;
    char *group;
} FileInfo;

typedef struct {
    FileInfo *items;
    size_t count;
    size_t capacity;
} Files;

typedef enum {
    FT_REGULAR,
    FT_DIRECTORY,
    FT_OTHER,
} File_Type;

Errno type_of_file(const char *file_path, File_Type *ft);
Errno read_entire_file(const char *file_path, String_Builder *sb);
Errno write_entire_file(const char *file_path, const char *buf, size_t buf_size);
Errno read_entire_dir(const char *dir_path, Files *files);

void get_file_info(const char *dir, const char *filename, FileInfo *info);



bool is_hex_digit(char c);
Vec4f hex_to_vec4f(uint32_t color);


#endif // COMMON_H_
