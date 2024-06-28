#version 330 core

uniform sampler2D image;

in vec4 out_color;
in vec2 out_uv;

void main() {
    float d = texture(image, out_uv).r;
    float aaf = fwidth(d);
    float alpha = smoothstep(0.5 - aaf, 0.5 + aaf, d);
    gl_FragColor = vec4(out_color.rgb, alpha);
}



// Dumb shadow
/* look good only with radon font */
/* #version 330 core */

/* uniform sampler2D image; */

/* in vec4 out_color; */
/* in vec2 out_uv; */

/* void main() { */
/*     // Shadow properties */
/*     vec2 shadowOffset = vec2(0.001, -0.001); // Very close offset for the shadow */
/*     vec4 shadowColor = vec4(0.0, 0.0, 0.0, 0.7); // Black shadow with decent opacity */

/*     // Calculate distance for shadow and text */
/*     float d = texture(image, out_uv).r; */

/*     float aaf = fwidth(d); */
/*     float alpha = smoothstep(0.5 - aaf, 0.5 + aaf, d); */

/*     // Basic blur approximation for shadow */
/*     float blurRadius = 0.0005; // Adjust this value for more/less blur */
/*     float shadowAlpha = 0.0; */
/*     for (float x = -blurRadius; x <= blurRadius; x += 0.001) { */
/*         for (float y = -blurRadius; y <= blurRadius; y += 0.001) { */
/*             float dShadow = texture(image, out_uv + shadowOffset + vec2(x, y)).r; */
/*             shadowAlpha += smoothstep(0.5 - aaf, 0.5 + aaf, dShadow); */
/*         } */
/*     } */
/*     shadowAlpha = shadowAlpha / ((2.0 * blurRadius / 0.001 + 1.0) * (2.0 * blurRadius / 0.001 + 1.0)); */
/*     shadowAlpha *= shadowColor.a; */

/*     // Mix shadow and text */
/*     vec4 textColor = vec4(out_color.rgb, alpha); */
/*     vec4 shadow = vec4(shadowColor.rgb, shadowAlpha); */
/*     gl_FragColor = mix(shadow, textColor, alpha); */
/* } */


// idk
/* #version 330 core */

/* uniform sampler2D image; */
/* uniform float time; // Time variable for animation */

/* in vec4 out_color; // Base text color */
/* in vec2 out_uv; // Texture coordinates */

/* void main() { */
/*     float d = texture(image, out_uv).r; // Distance field value */
/*     float aaf = fwidth(d); // Antialiasing factor */

/*     // Outline settings */
/*     float outlineSize = 0.09; // Width of the outline */
/*     float movingSegmentLength = 0.8; // Length of the moving segment */
/*     float speed = 0.5; // Speed of the segment's movement */

/*     // Calculate position of the moving segment */
/*     float segmentPosition = mod(time * speed, 1.0); */

/*     // Calculate outline alpha based on SDF */
/*     float outlineAlpha = smoothstep(0.5 - outlineSize - aaf, 0.5 - outlineSize, d) - */
/*                          smoothstep(0.5 + outlineSize, 0.5 + outlineSize + aaf, d); */

/*     // Determine if we're within the moving segment */
/*     float sdfCoord = (atan(out_uv.y - 0.5, out_uv.x - 0.5) / 3.14159265 + 1.0) * 0.5; */
/*     bool inMovingSegment = abs(sdfCoord - segmentPosition) < movingSegmentLength * 0.5 || */
/*                            abs(sdfCoord - segmentPosition - 1.0) < movingSegmentLength * 0.5; */

/*     // Mix colors: text, outline, and moving segment */
/*     vec3 outlineColor = vec3(0, 1, 0); // Green for the moving segment */
/*     vec4 textColor = vec4(out_color.rgb, smoothstep(0.5 - aaf, 0.5 + aaf, d)); */
/*     vec4 segmentColor = mix(vec4(outlineColor, outlineAlpha), textColor, float(!inMovingSegment)); */
    
/*     gl_FragColor = segmentColor; */
/* } */
