/* #version 330 core */

/* in vec4 out_color; */

/* void main() { */
/*     gl_FragColor = out_color; */
/* } */


#version 330 core

in vec4 out_color;  // Color input for the object.
in vec2 frag_coord;  // The coordinate of the current fragment in screen space.

uniform vec2 light_position;  // The position of the light source in screen space.

out vec4 frag_color;  // Output color of the fragment.

void main() {
    // Calculate the distance between the fragment and the light source.
    float distance_to_light = length(frag_coord - light_position);

    // Define a shadow radius and adjust the shadow intensity.
    float shadow_radius = 100.0;
    float shadow_intensity = 0.3;  // Adjust this value to control the shadow intensity.

    // Calculate the shadow factor based on distance.
    float shadow_factor = smoothstep(shadow_radius, shadow_radius + 1.0, distance_to_light);

    // Apply the shadow factor to darken the input color.
    vec3 fragment_color = out_color.rgb * (1.0 - shadow_intensity * shadow_factor);

    frag_color = vec4(fragment_color, out_color.a);
}




// GRADIENT
/* #version 330 core */

/* in vec2 fragTexCoord; // Texture coordinate, if you're using textures */
/* uniform float time;   // Uniform variable for time */
/* uniform vec2 cursorPos; // Cursor position in screen coordinates */
/* uniform vec2 resolution; // Screen resolution */

/* out vec4 color; */

/* void main() { */
/*     // Calculate normalized coordinates */
/*     vec2 uv = fragTexCoord / resolution; */

/*     // Create a dynamic gradient based on time */
/*     vec3 gradient = 0.5 + 0.9 * cos(time + uv.xyx + vec3(0, 2, 4)); */

/*     // Create a glow effect */
/*     float dist = distance(uv, cursorPos / resolution); */
/*     float glow = exp(20.0 * dist); */

/*     // Combine the gradient and glow */
/*     vec3 finalColor = gradient * glow; */

/*     // Output the color */
/*     color = vec4(finalColor, 0.7); // Fully opaque */
/* } */


