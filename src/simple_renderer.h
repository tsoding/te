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
    SHADER_FOR_EPICNESS, // This is the one that does that cool rainbowish animation
    COUNT_SIMPLE_SHADERS,
} Simple_Shader;

typedef struct {
    Vec2f pos;
    float scale;
    float scale_vel;
    Vec2f vel;
} Simple_Camera;

typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint programs[COUNT_SIMPLE_SHADERS];
    Simple_Shader current_shader;

    GLint uniforms[COUNT_UNIFORM_SLOTS];
    Simple_Vertex verticies[SIMPLE_VERTICIES_CAP];
    size_t verticies_count;

    Vec2f resolution;
    float time;

    Simple_Camera cam;
} Simple_Renderer;

void simple_renderer_init(Simple_Renderer *sr);

void simple_renderer_reload_shaders(Simple_Renderer *sr);

void simple_renderer_vertex(Simple_Renderer *sr,
                            Vec2f p, Vec4f c, Vec2f uv);
void simple_renderer_set_shader(Simple_Renderer *sr, Simple_Shader shader);
void simple_renderer_triangle(Simple_Renderer *sr,
                              Vec2f p0, Vec2f p1, Vec2f p2,
                              Vec4f c0, Vec4f c1, Vec4f c2,
                              Vec2f uv0, Vec2f uv1, Vec2f uv2);
void simple_renderer_quad(Simple_Renderer *sr,
                          Vec2f p0, Vec2f p1, Vec2f p2, Vec2f p3,
                          Vec4f c0, Vec4f c1, Vec4f c2, Vec4f c3,
                          Vec2f uv0, Vec2f uv1, Vec2f uv2, Vec2f uv3);
void simple_renderer_solid_rect(Simple_Renderer *sr, Vec2f p, Vec2f s, Vec4f c);
void simple_renderer_image_rect(Simple_Renderer *sr, Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs, Vec4f c);
void simple_renderer_flush(Simple_Renderer *sr);
void simple_renderer_sync(Simple_Renderer *sr);
void simple_renderer_draw(Simple_Renderer *sr);

#endif  // SIMPLE_RENDERER_H_
