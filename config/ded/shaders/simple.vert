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
    gl_Position = vec4(camera_project(position), 0, 1);
    out_color = color;
    out_uv = uv;
}
