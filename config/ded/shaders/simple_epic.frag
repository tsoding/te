#version 330 core

uniform float time;
uniform vec2 resolution;
uniform sampler2D image;

in vec2 out_uv;

vec3 hsl2rgb(vec3 c) {
    vec3 rgb = clamp(abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0);
    return c.z + c.y * (rgb-0.5)*(1.0-abs(2.0*c.z-1.0));
}

void main() {
    vec4 tc = texture(image, out_uv);
    float d = tc.r;
    float aaf = fwidth(d);
    float alpha = smoothstep(0.5 - aaf, 0.5 + aaf, d);

    vec2 frag_uv = gl_FragCoord.xy / resolution;

    // Dynamic color-shifting aura
    vec3 auraColor = hsl2rgb(vec3(mod(time * 0.2 + frag_uv.y, 1.0), 0.5, 0.5));

    // Shimmering gradient across the text
    float shimmer = (sin(time * 3.0 + frag_uv.x * 10.0) + 1.0) * 0.5;
    vec3 shimmerColor = mix(vec3(1.0, 0.8, 0.6), vec3(0.6, 0.8, 1.0), shimmer);

    vec3 finalColor = mix(auraColor, shimmerColor, d);
    gl_FragColor = vec4(finalColor, alpha);
}
