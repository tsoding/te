#version 330 core

uniform sampler2D image; // The off-screen texture with the cursor
uniform float blurSize;  // The size of the blur effect (experiment with this value)

in vec2 out_uv;

const float offset[5] = float[](0.0, 1.333, 2.666, 4.0, 5.333); // Offsets for Gaussian blur

void main() {
    vec4 sum = texture(image, out_uv) * 0.2941; // Central sample (weight is highest)

    sum += texture(image, out_uv + vec2(blurSize * offset[1], 0.0)) * 0.2353;
    sum += texture(image, out_uv - vec2(blurSize * offset[1], 0.0)) * 0.2353;
    sum += texture(image, out_uv + vec2(blurSize * offset[2], 0.0)) * 0.1176;
    sum += texture(image, out_uv - vec2(blurSize * offset[2], 0.0)) * 0.1176;
    // ... Add more samples if needed

    gl_FragColor = sum;
}
