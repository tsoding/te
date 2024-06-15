/* WAWE */
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
/*     vec4 projected_position = vec4(camera_project(position), 0.0, 1.0); */

/*     // Adding a displacement effect that varies with time in screen space */
/*     projected_position.x += sin(projected_position.y + time) * 0.22; // Horizontal wave */
/*     projected_position.y += cos(projected_position.x + time) * 0.02; // Vertical wave */

/*     gl_Position = projected_position; */

/*     out_color = color; */
/*     out_uv = uv; */
/* } */



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


// GUITAR HERO V2
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

/*     // Adjust for more or less perspective, and limit the effect as y approaches the edges */
/*     float perspectiveDepth = -0.45; */
/*     float clamped_y = clamp(projected_point.y, -0.95, 0.95); // Clamping to avoid extreme values at the edges */
/*     float depth = 1.0 / (1.0 - clamped_y * perspectiveDepth); */

/*     projected_point.x *= depth; */
/*     projected_point.y *= depth; // Apply depth scaling uniformly */

/*     // Convert back to vec4 */
/*     vec4 projected_position = vec4(projected_point, 0.0, 1.0); */

/*     gl_Position = projected_position; */

/*     out_color = color; */
/*     out_uv = uv; */
/* } */



// RIPPLE
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

/* vec2 camera_project(vec2 point) { */
/*     return 2.0 * (point - camera_pos) * camera_scale / resolution; */
/* } */

/* void main() { */
/*     // Apply camera projection */
/*     vec2 projected_point = camera_project(position); */

/*     // Calculate the center point for the ripple effect (center of the screen) */
/*     vec2 center = vec2(0.0, 0.0); */

/*     // Calculate the direction and distance from the current point to the ripple center */
/*     vec2 to_center = projected_point - center; */
/*     float distance = length(to_center); */

/*     // Apply a ripple displacement based on distance and time */
/*     float ripple_frequency = 10.0; // Adjust for ripple tightness */
/*     float ripple_speed = 2.0; // Adjust for ripple speed */
/*     float ripple_amplitude = 0.05; // Adjust for ripple strength */

/*     float ripple = sin(distance * ripple_frequency - time * ripple_speed) * ripple_amplitude; */

/*     // Displace the projected point radially from the center */
/*     projected_point += normalize(to_center) * ripple; */

/*     // Convert back to vec4 and set position */
/*     vec4 projected_position = vec4(projected_point, 0.0, 1.0); */
/*     gl_Position = projected_position; */

/*     out_color = color; */
/*     out_uv = uv; */
/* } */



//Jitter 
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

/* vec2 camera_project(vec2 point) { */
/*     return 2.0 * (point - camera_pos) * camera_scale / resolution; */
/* } */

/* void main() { */
/*     // Apply camera projection */
/*     vec2 projected_point = camera_project(position); */

/*     // Apply a jitter effect based on time to create flickering */
/*     float jitter_intensity = 0.02; // Adjust for desired flicker strength */
/*     projected_point.x += (sin(position.y * 10.0 + time) * jitter_intensity); */
/*     projected_point.y += (cos(position.x * 10.0 + time) * jitter_intensity); */

/*     // Convert back to vec4 and set position */
/*     vec4 projected_position = vec4(projected_point, 0.0, 1.0); */
/*     gl_Position = projected_position; */

/*     out_color = color; */
/*     out_uv = uv; */
/* } */


// WOBBLE
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

vec2 camera_project(vec2 point) {
    return 2.0 * (point - camera_pos) * camera_scale / resolution;
}

void main() {
    // Apply camera projection
    vec2 projected_point = camera_project(position);

    // Apply a wobble effect using a circular pattern around the original point
    float wobble_amplitude = 0.05; // Adjust for the wobble distance
    float wobble_speed = 5.0; // Adjust for the wobble speed

    projected_point.x += sin(time * wobble_speed + projected_point.y) * wobble_amplitude;
    projected_point.y += cos(time * wobble_speed + projected_point.x) * wobble_amplitude;

    // Convert back to vec4 and set position
    vec4 projected_position = vec4(projected_point, 0.0, 1.0);
    gl_Position = projected_position;

    out_color = color;
    out_uv = uv;
}



