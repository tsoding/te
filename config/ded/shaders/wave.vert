/* WAWE */
#version 330 core

uniform vec2 resolution;
uniform float time;
uniform float camera_scale;
uniform vec2 camera_pos;

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv;

out vec4 out_color;
out vec2 out_uv;

vec2 camera_project(vec2 point)
{
    return 2.0 * (point - camera_pos) * camera_scale / resolution;
}

void main() {
    // Apply camera projection first
    vec4 projected_position = vec4(camera_project(position), 0.0, 1.0);

    // Adding a displacement effect that varies with time in screen space
    projected_position.x += sin(projected_position.y + time) * 0.22; // Horizontal wave
    projected_position.y += cos(projected_position.x + time) * 0.02; // Vertical wave

    gl_Position = projected_position;

    out_color = color;
    out_uv = uv;
}



// GUITAR HERO
/* #version 330 core */

/* uniform vec2 resolution; */
/* uniform float time; */
/* uniform float camera_scale; */
/* uniform vec2 camera_pos; */

/* layout(location = 0) in vec2 position; */
/* layout(location = 1) in vec4 color; */
/* layout(location = 2) in vec2 uv; */

/* out vec4 out_color; */
/* out vec2 out_uv; */

/* vec2 camera_project(vec2 point) */
/* { */
/*     return 2.0 * (point - camera_pos) * camera_scale / resolution; */
/* } */

/* void main() { */
/*     // Apply camera projection first */
/*     vec2 projected_point = camera_project(position); */

/*     // Apply a static perspective transformation */
/*     float perspectiveDepth = -0.45; // Adjust for more or less perspective */
/*     float depth = 1.0 / (1.0 - projected_point.y * perspectiveDepth); */

/*     projected_point *= depth; */

/*     // Convert back to vec4 */
/*     vec4 projected_position = vec4(projected_point, 0.0, 1.0); */

/*     gl_Position = projected_position; */

/*     out_color = color; */
/*     out_uv = uv; */
/* } */
