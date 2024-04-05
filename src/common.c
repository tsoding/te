#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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
#define FILELIB_IMPL
#include <filelib.h>
#define MINICONF_IMPL
#include <miniconf.h>
#include <memlib.h>

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

    // TODO: why are there extra nulls at the end of the buf (buf_size)??
    // that's why we strlen it here to get the size without these nulls
    size_t real_buf_size = strlen(buf);
    if (buf_size < real_buf_size)
        real_buf_size = buf_size;
    fwrite(buf, 1, real_buf_size, f);
    if (get_last(buf, real_buf_size) != '\n') {
        fputc('\n', f);
    }
    if (ferror(f)) return_defer(errno);

defer:
    if (f) fclose(f);
    return result;
}

Errno read_entire_file(const char *file_path, String_Builder *sb)
{
    FILE *f = fopen(file_path, "r+");
    if (f == NULL)
         return 1;

    int c;
    while ((c = fgetc(f)) != EOF && c != '\0') {
        if (c == '\t') {
            da_append(sb, ' ');
            da_append(sb, ' ');
            da_append(sb, ' ');
            da_append(sb, ' ');
        } else if (c == '\r') {
            continue;
        } else {
            da_append(sb, (char) c);
        }
    }

    fclose(f);
    return 0;
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
    DWORD file_obj_type = GetFileAttributesA(file_path);
    if (file_obj_type & FILE_ATTRIBUTE_DIRECTORY) {
        *ft = FT_DIRECTORY;
    }
    // I have no idea why, but a 'normal' file is considered an archive file?
    else if (file_obj_type & FILE_ATTRIBUTE_ARCHIVE) {
        *ft = FT_REGULAR;
    }
    else {
        *ft = FT_OTHER;
    }
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
