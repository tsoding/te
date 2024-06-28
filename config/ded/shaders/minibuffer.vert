#version 330 core

uniform vec2 resolution;
uniform float time;

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv;

out vec4 out_color;
out vec2 out_uv;

const float scaleFactor = 0.25;

vec2 simple_project(vec2 point) {
    // Project the point directly based on the resolution
    return (2.0 * point / resolution) - vec2(1.0);
}

void main() {
    // Apply the scale factor to the position
    vec2 scaledPosition = position * scaleFactor;

    // Use the simple projection method without camera transformations
    gl_Position = vec4(simple_project(scaledPosition), 0.0, 1.0);
    out_color = color;
    out_uv = uv;
}
