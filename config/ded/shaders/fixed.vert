#version 330 core

uniform vec2 resolution;
uniform float time;

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv;

out vec4 out_color;
out vec2 out_uv;

vec2 simple_project(vec2 point)
{
    // Project the point directly based on the resolution
    return (2.0 * point / resolution) - vec2(1.0);
}

void main() {
    // Use the simple projection method without camera transformations
    gl_Position = vec4(simple_project(position), 0.0, 1.0);
    out_color = color;
    out_uv = uv;
}
