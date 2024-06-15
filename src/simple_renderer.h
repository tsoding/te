#ifndef SIMPLE_RENDERER_H_
#define SIMPLE_RENDERER_H_

#include <assert.h>

#define GLEW_STATIC
#include <GL/glew.h>

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>

#include "./la.h"

typedef enum {
    UNIFORM_SLOT_TIME = 0,
    UNIFORM_SLOT_RESOLUTION,
    UNIFORM_SLOT_CAMERA_POS,
    UNIFORM_SLOT_CAMERA_SCALE,
    COUNT_UNIFORM_SLOTS,
} Uniform_Slot;

typedef enum {
    SIMPLE_VERTEX_ATTR_POSITION = 0,
    SIMPLE_VERTEX_ATTR_COLOR,
    SIMPLE_VERTEX_ATTR_UV,
} Simple_Vertex_Attr;

typedef struct {
    Vec2f position;
    Vec4f color;
    Vec2f uv;
} Simple_Vertex;

#define SIMPLE_VERTICIES_CAP (3*640*1000)

static_assert(SIMPLE_VERTICIES_CAP%3 == 0, "Simple renderer vertex capacity must be divisible by 3. We are rendring triangles after all.");

typedef enum {
    SHADER_FOR_COLOR = 0,
    SHADER_FOR_IMAGE,
    SHADER_FOR_TEXT,
    SHADER_FOR_GLOW,
    SHADER_FOR_EPICNESS,
    SHADER_FOR_RAINBOW,
    SHADER_FOR_CURSOR,
    VERTEX_SHADER_SIMPLE,
    VERTEX_SHADER_FIXED,
    VERTEX_SHADER_MINIBUFFER,
    VERTEX_SHADER_WAVE,
    COUNT_FRAGMENT_SHADERS,
    COUNT_VERTEX_SHADERS,
} Simple_Shader;

typedef struct {
    GLuint vao;
    GLuint vbo;
    /* GLuint programs[COUNT_SIMPLE_SHADERS]; */
    GLuint programs[COUNT_VERTEX_SHADERS][COUNT_FRAGMENT_SHADERS];
    Simple_Shader current_shader;

    int current_vertex_shader_index;
    int current_fragment_shader_index;

    GLint uniforms[COUNT_UNIFORM_SLOTS];
    Simple_Vertex verticies[SIMPLE_VERTICIES_CAP];
    size_t verticies_count;

    Vec2f resolution;
    float time;

    Vec2f camera_pos;
    float camera_scale;
    float camera_scale_vel;
    Vec2f camera_vel;
} Simple_Renderer;


// old
/* extern const char *vert_shader_file_path; */


#define MAX_SHADER_PATH_LENGTH 256
/* extern char vert_shader_file_path[MAX_SHADER_PATH_LENGTH]; */
extern char vert_shader_file_path[COUNT_VERTEX_SHADERS][MAX_SHADER_PATH_LENGTH];






void simple_renderer_init(Simple_Renderer *sr);
void simple_renderer_reload_shaders(Simple_Renderer *sr);
void simple_renderer_vertex(Simple_Renderer *sr, Vec2f p, Vec4f c, Vec2f uv);
void simple_renderer_set_shader(Simple_Renderer *sr, int vertexShaderIndex,
                                int fragmentShaderIndex);
void simple_renderer_triangle(Simple_Renderer *sr, Vec2f p0, Vec2f p1, Vec2f p2,
                              Vec4f c0, Vec4f c1, Vec4f c2, Vec2f uv0,
                              Vec2f uv1, Vec2f uv2);
void simple_renderer_quad(Simple_Renderer *sr, Vec2f p0, Vec2f p1, Vec2f p2,
                          Vec2f p3, Vec4f c0, Vec4f c1, Vec4f c2, Vec4f c3,
                          Vec2f uv0, Vec2f uv1, Vec2f uv2, Vec2f uv3);
void simple_renderer_solid_rect(Simple_Renderer *sr, Vec2f p, Vec2f s, Vec4f c);
void simple_renderer_image_rect(Simple_Renderer *sr, Vec2f p, Vec2f s,
                                Vec2f uvp, Vec2f uvs, Vec4f c);
void simple_renderer_flush(Simple_Renderer * sr);
void simple_renderer_sync(Simple_Renderer * sr);
void simple_renderer_draw(Simple_Renderer * sr);

// ADDED
void initialize_shader_paths();
void simple_renderer_circle(Simple_Renderer *sr, Vec2f center, float radius, Vec4f color, int segments);

    /* const char *resolve_shader_path(const char *shader_file_name); */

#endif  // SIMPLE_RENDERER_H_
