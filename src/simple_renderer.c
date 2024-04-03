#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "./simple_renderer.h"
#include "./common.h"

#define vert_shader_file_path "./shaders/simple.vert"

static_assert(COUNT_SIMPLE_SHADERS == 4, "The amount of fragment shaders has changed");
const char *frag_shader_file_paths[COUNT_SIMPLE_SHADERS] = {
    [SHADER_FOR_COLOR] = "./shaders/simple_color.frag",
    [SHADER_FOR_IMAGE] = "./shaders/simple_image.frag",
    [SHADER_FOR_TEXT] = "./shaders/simple_text.frag",
    [SHADER_FOR_EPICNESS] = "./shaders/simple_epic.frag",
};

static const char *shader_type_as_cstr(GLuint shader)
{
    switch (shader) {
    case GL_VERTEX_SHADER:
        return "GL_VERTEX_SHADER";
    case GL_FRAGMENT_SHADER:
        return "GL_FRAGMENT_SHADER";
    default:
        return "(Unknown)";
    }
}

static bool compile_shader_source(const GLchar *source, GLenum shader_type, GLuint *shader)
{
    *shader = glCreateShader(shader_type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);

    GLint compiled = 0;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLchar message[1024];
        GLsizei message_size = 0;
        glGetShaderInfoLog(*shader, sizeof(message), &message_size, message);
        fprintf(stderr, "ERROR: could not compile %s\n", shader_type_as_cstr(shader_type));
        fprintf(stderr, "%.*s\n", message_size, message);
        return false;
    }

    return true;
}

static bool compile_shader_file(const char *file_path, GLenum shader_type, GLuint *shader)
{
    bool result = true;

    String_Builder source = {0};
    Errno err = read_entire_file(file_path, &source);
    if (err != 0) {
        fprintf(stderr, "ERROR: failed to load `%s` shader file: %s\n", file_path, strerror(errno));
        return_defer(false);
    }
    sb_append_null(&source);

    if (!compile_shader_source(source.items, shader_type, shader)) {
        fprintf(stderr, "ERROR: failed to compile `%s` shader file\n", file_path);
        return_defer(false);
    }
defer:
    free(source.items);
    return result;
}

static void attach_shaders_to_program(GLuint *shaders, size_t shaders_count, GLuint program)
{
    for (size_t i = 0; i < shaders_count; ++i) {
        glAttachShader(program, shaders[i]);
    }
}

static bool link_program(GLuint program, const char *file_path, size_t line)
{
    glLinkProgram(program);

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLsizei message_size = 0;
        GLchar message[1024];

        glGetProgramInfoLog(program, sizeof(message), &message_size, message);
        fprintf(stderr, "%s:%zu: Program Linking: %.*s\n", file_path, line, message_size, message);
    }

    return linked;
}

typedef struct {
    Uniform_Slot slot;
    const char *name;
} Uniform_Def;

static_assert(COUNT_UNIFORM_SLOTS == 4, "The amount of the shader uniforms have change. Please update the definition table accordingly");
static const Uniform_Def uniform_defs[COUNT_UNIFORM_SLOTS] = {
    [UNIFORM_SLOT_TIME] = {
        .slot = UNIFORM_SLOT_TIME,
        .name = "time",
    },
    [UNIFORM_SLOT_RESOLUTION] = {
        .slot = UNIFORM_SLOT_RESOLUTION,
        .name = "resolution",
    },
    [UNIFORM_SLOT_CAMERA_POS] = {
        .slot = UNIFORM_SLOT_CAMERA_POS,
        .name = "camera_pos",
    },
    [UNIFORM_SLOT_CAMERA_SCALE] = {
        .slot = UNIFORM_SLOT_CAMERA_SCALE,
        .name = "camera_scale",
    },
};


static void get_uniform_location(GLuint program, GLint locations[COUNT_UNIFORM_SLOTS])
{
    for (Uniform_Slot slot = 0; slot < COUNT_UNIFORM_SLOTS; ++slot) {
        locations[slot] = glGetUniformLocation(program, uniform_defs[slot].name);
    }
}

void simple_renderer_init(Simple_Renderer *sr)
{
    sr->cam.scale = 3.0f;

    {
        glGenVertexArrays(1, &sr->vao);
        glBindVertexArray(sr->vao);

        glGenBuffers(1, &sr->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, sr->vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(sr->verticies), sr->verticies, GL_DYNAMIC_DRAW);

        // position
        glEnableVertexAttribArray(SIMPLE_VERTEX_ATTR_POSITION);
        glVertexAttribPointer(
            SIMPLE_VERTEX_ATTR_POSITION,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Simple_Vertex),
            (GLvoid *) offsetof(Simple_Vertex, position));

        // color
        glEnableVertexAttribArray(SIMPLE_VERTEX_ATTR_COLOR);
        glVertexAttribPointer(
            SIMPLE_VERTEX_ATTR_COLOR,
            4,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Simple_Vertex),
            (GLvoid *) offsetof(Simple_Vertex, color));

        // uv
        glEnableVertexAttribArray(SIMPLE_VERTEX_ATTR_UV);
        glVertexAttribPointer(
            SIMPLE_VERTEX_ATTR_UV,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Simple_Vertex),
            (GLvoid *) offsetof(Simple_Vertex, uv));
    }

    GLuint shaders[2] = {0};

    if (!compile_shader_file(vert_shader_file_path, GL_VERTEX_SHADER, &shaders[0])) {
        exit(1);
    }

    for (int i = 0; i < COUNT_SIMPLE_SHADERS; ++i) {
        if (!compile_shader_file(frag_shader_file_paths[i], GL_FRAGMENT_SHADER, &shaders[1])) {
            exit(1);
        }
        sr->programs[i] = glCreateProgram();
        attach_shaders_to_program(shaders, sizeof(shaders) / sizeof(shaders[0]), sr->programs[i]);
        if (!link_program(sr->programs[i], __FILE__, __LINE__)) {
            exit(1);
        }
        glDeleteShader(shaders[1]);
    }
    glDeleteShader(shaders[0]);
}

void simple_renderer_reload_shaders(Simple_Renderer *sr)
{
    GLuint programs[COUNT_SIMPLE_SHADERS];
    GLuint shaders[2] = {0};

    bool ok = true;

    if (!compile_shader_file(vert_shader_file_path, GL_VERTEX_SHADER, &shaders[0])) {
        ok = false;
    }

    for (int i = 0; i < COUNT_SIMPLE_SHADERS; ++i) {
        if (!compile_shader_file(frag_shader_file_paths[i], GL_FRAGMENT_SHADER, &shaders[1])) {
            ok = false;
        }
        programs[i] = glCreateProgram();
        attach_shaders_to_program(shaders, sizeof(shaders) / sizeof(shaders[0]), programs[i]);
        if (!link_program(programs[i], __FILE__, __LINE__)) {
            ok = false;
        }
        glDeleteShader(shaders[1]);
    }
    glDeleteShader(shaders[0]);

    if (ok) {
        for (int i = 0; i < COUNT_SIMPLE_SHADERS; ++i) {
            glDeleteProgram(sr->programs[i]);
            sr->programs[i] = programs[i];
        }
        printf("Reloaded shaders successfully!\n");
    } else {
        for (int i = 0; i < COUNT_SIMPLE_SHADERS; ++i) {
            glDeleteProgram(programs[i]);
        }
    }
}

// TODO: Don't render triples of verticies that form a triangle that is completely outside of the screen
//
// Ideas on how to check if a triangle is outside of the screen:
// 1. Apply camera transformations to the triangle.
// 2. Form an axis-aligned boundary box (AABB) of the triangle.
// 3. Check if the Triangle AABB does not intersect the Screen AABB.
//
// This might not be what we want at the end of the day, though. Because in case of a lot of triangles we
// end up iterating each of them at least once and doing camera trasformations on the CPU (which is
// something we do on GPU already).
//
// It would be probably better if such culling occurred on a higher level of abstractions. For example
// in the Editor. For instance, if the Editor noticed that the line it is currently rendering is
// below the screen, it should stop rendering the rest of the text, thus never calling
// simple_renderer_vertex() for a potentially large amount of verticies in the first place.
void simple_renderer_vertex(Simple_Renderer *sr, Vec2f p, Vec4f c, Vec2f uv)
{
#if 0
    // TODO: flush the renderer on vertex buffer overflow instead firing the assert
    if (sr->verticies_count >= SIMPLE_VERTICIES_CAP) simple_renderer_flush(sr);
#else
    // NOTE: it is better to just crash the app in this case until the culling described
    // above is sorted out.
    assert(sr->verticies_count < SIMPLE_VERTICIES_CAP);
#endif
    Simple_Vertex *last = &sr->verticies[sr->verticies_count];
    last->position = p;
    last->color    = c;
    last->uv       = uv;
    sr->verticies_count += 1;
}

void simple_renderer_triangle(Simple_Renderer *sr,
                              Vec2f p0, Vec2f p1, Vec2f p2,
                              Vec4f c0, Vec4f c1, Vec4f c2,
                              Vec2f uv0, Vec2f uv1, Vec2f uv2)
{
    simple_renderer_vertex(sr, p0, c0, uv0);
    simple_renderer_vertex(sr, p1, c1, uv1);
    simple_renderer_vertex(sr, p2, c2, uv2);
}

// 2-3
// |\|
// 0-1
void simple_renderer_quad(Simple_Renderer *sr,
                          Vec2f p0, Vec2f p1, Vec2f p2, Vec2f p3,
                          Vec4f c0, Vec4f c1, Vec4f c2, Vec4f c3,
                          Vec2f uv0, Vec2f uv1, Vec2f uv2, Vec2f uv3)
{
    simple_renderer_triangle(sr, p0, p1, p2, c0, c1, c2, uv0, uv1, uv2);
    simple_renderer_triangle(sr, p1, p2, p3, c1, c2, c3, uv1, uv2, uv3);
}

void simple_renderer_image_rect(Simple_Renderer *sr, Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs, Vec4f c)
{
    simple_renderer_quad(
        sr,
        p, vec2f_add(p, vec2f(s.x, 0)), vec2f_add(p, vec2f(0, s.y)), vec2f_add(p, s),
        c, c, c, c,
        uvp, vec2f_add(uvp, vec2f(uvs.x, 0)), vec2f_add(uvp, vec2f(0, uvs.y)), vec2f_add(uvp, uvs));
}

void simple_renderer_solid_rect(Simple_Renderer *sr, Vec2f p, Vec2f s, Vec4f c)
{
    Vec2f uv = vec2fs(0);
    simple_renderer_quad(
        sr,
        p, vec2f_add(p, vec2f(s.x, 0)), vec2f_add(p, vec2f(0, s.y)), vec2f_add(p, s),
        c, c, c, c,
        uv, uv, uv, uv);
}

void simple_renderer_sync(Simple_Renderer *sr)
{
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    sr->verticies_count * sizeof(Simple_Vertex),
                    sr->verticies);
}

void simple_renderer_draw(Simple_Renderer *sr)
{
    glDrawArrays(GL_TRIANGLES, 0, sr->verticies_count);
}

void simple_renderer_set_shader(Simple_Renderer *sr, Simple_Shader shader)
{
    sr->current_shader = shader;
    glUseProgram(sr->programs[sr->current_shader]);
    get_uniform_location(sr->programs[sr->current_shader], sr->uniforms);
    glUniform2f(sr->uniforms[UNIFORM_SLOT_RESOLUTION], sr->resolution.x, sr->resolution.y);
    glUniform1f(sr->uniforms[UNIFORM_SLOT_TIME], sr->time);
    glUniform2f(sr->uniforms[UNIFORM_SLOT_CAMERA_POS], sr->cam.pos.x, sr->cam.pos.y);
    glUniform1f(sr->uniforms[UNIFORM_SLOT_CAMERA_SCALE], sr->cam.scale);
}

void simple_renderer_flush(Simple_Renderer *sr)
{
    simple_renderer_sync(sr);
    simple_renderer_draw(sr);
    sr->verticies_count = 0;
}
