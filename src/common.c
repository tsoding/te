#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "editor.h"

#ifdef _WIN32
#    define MINIRENT_IMPLEMENTATION
#    include <minirent.h>
#else
#    include <dirent.h>
#    include <sys/types.h>
#    include <sys/stat.h>
#    include <unistd.h>
#endif // _WIN32

#include "common.h"
#define ARENA_IMPLEMENTATION
#include "./arena.h"
#define SV_IMPLEMENTATION
#include "sv.h"

static Arena temporary_arena = {0};

char *temp_strdup(const char *s)
{
    size_t n = strlen(s);
    char *ds = arena_alloc(&temporary_arena, n + 1);
    memcpy(ds, s, n);
    ds[n] = '\0';
    return ds;
}

void temp_reset(void)
{
    arena_reset(&temporary_arena);
}

Errno read_entire_dir(const char *dir_path, Files *files)
{
    Errno result = 0;
    DIR *dir = NULL;

    dir = opendir(dir_path);
    if (dir == NULL) {
        return_defer(errno);
    }

    errno = 0;
    struct dirent *ent = readdir(dir);
    while (ent != NULL) {
        da_append(files, temp_strdup(ent->d_name));
        ent = readdir(dir);
    }

    if (errno != 0) {
        return_defer(errno);
    }

defer:
    if (dir) closedir(dir);
    return result;
}

Errno write_entire_file(const char *file_path, const char *buf, size_t buf_size)
{
    Errno result = 0;
    FILE *f = NULL;

    f = fopen(file_path, "wb");
    if (f == NULL) return_defer(errno);

    fwrite(buf, 1, buf_size, f);
    if (ferror(f)) return_defer(errno);

defer:
    if (f) fclose(f);
    return result;
}

static Errno file_size(FILE *file, size_t *size)
{
    long saved = ftell(file);
    if (saved < 0) return errno;
    if (fseek(file, 0, SEEK_END) < 0) return errno;
    long result = ftell(file);
    if (result < 0) return errno;
    if (fseek(file, saved, SEEK_SET) < 0) return errno;
    *size = (size_t) result;
    return 0;
}

Errno create_new_file_here(const char *file_name)
{
    Errno result = 0;

    FILE *f = fopen(file_name, "wb");
    if (f == NULL)
        return errno;

    fclose(f);
    return result;
}


/* Errno read_entire_file(const char *file_path, String_Builder *sb) */
/* { */
/*     Errno result = 0; */
/*     FILE *f = NULL; */

/*     f = fopen(file_path, "r"); */
/*     if (f == NULL) return_defer(errno); */

/*     size_t size; */
/*     Errno err = file_size(f, &size); */
/*     if (err != 0) return_defer(err); */

/*     if (sb->capacity < size) { */
/*         sb->capacity = size; */
/*         sb->items = realloc(sb->items, sb->capacity*sizeof(*sb->items)); */
/*         assert(sb->items != NULL && "Buy more RAM lol"); */
/*     } */

/*     fread(sb->items, size, 1, f); */
/*     if (ferror(f)) return_defer(errno); */
/*     sb->count = size; */

/* defer: */
/*     if (f) fclose(f); */
/*     return result; */
/* } */


// Convert tabs into 4 spaces
// and ignore carriage returns
/* Errno read_entire_file(const char *file_path, String_Builder *sb) { */
/*     FILE *f = fopen(file_path, "r"); */
/*     if (f == NULL) */
/*          return 1; */

/*     int c; */
/*     while ((c = fgetc(f)) != EOF && c != '\0') { */
/*         if (c == '\t') { */
/*             da_append(sb, ' '); */
/*             da_append(sb, ' '); */
/*             da_append(sb, ' '); */
/*             da_append(sb, ' '); */
/*         } else if (c == '\r') { */
/*             continue; */
/*         } else { */
/*             da_append(sb, (char) c); */
/*         } */
/*     } */

/*     fclose(f); */
/*     return 0; */
/* } */


// Convert tabs into 4 spaces
// and ignore carriage returns
Errno read_entire_file(const char *file_path, String_Builder *sb) {
    FILE *f = fopen(file_path, "r+");
    if (f == NULL) {
        if (errno == EACCES) {
            // Retry opening in read-only mode
            f = fopen(file_path, "r");
            if (f == NULL) return 1; // If opening still fails, return error
            readonly = true;
        } else {
            return 1; // Other errors, return error
        }
    }

    int c;
    while ((c = fgetc(f)) != EOF && c != '\0') {
        if (c == '\t') {
            for (int i = 0; i < indentation; i++) {
                da_append(sb, ' ');
            }
        } else if (c == '\r') {
            continue; // Ignore carriage returns
        } else {
            da_append(sb, (char)c);
        }
    }

    fclose(f);
    return 0;
}




bool is_hex_digit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}


Vec4f hex_to_vec4f(uint32_t color)
{
    Vec4f result;
    uint32_t r = (color>>(3*8))&0xFF;
    uint32_t g = (color>>(2*8))&0xFF;
    uint32_t b = (color>>(1*8))&0xFF;
    uint32_t a = (color>>(0*8))&0xFF;
    result.x = r/255.0f;
    result.y = g/255.0f;
    result.z = b/255.0f;
    result.w = a/255.0f;
    return result;
}

Errno type_of_file(const char *file_path, File_Type *ft)
{
#ifdef _WIN32
#error "TODO: type_of_file() is not implemented for Windows"
#else
    struct stat sb = {0};
    if (stat(file_path, &sb) < 0) return errno;
    if (S_ISREG(sb.st_mode)) {
        *ft = FT_REGULAR;
    } else if (S_ISDIR(sb.st_mode)) {
        *ft = FT_DIRECTORY;
    } else {
        *ft = FT_OTHER;
    }
#endif
    return 0;
}

