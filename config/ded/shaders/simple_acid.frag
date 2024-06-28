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

    // Firefly movement: This simulates the movement of 3 "fireflies"
    float f1 = abs(sin(frag_uv.x * 10.0 + time));
    float f2 = abs(cos(frag_uv.y * 8.0 + time * 1.5));
    float f3 = abs(sin(frag_uv.x * 12.0 + frag_uv.y * 12.0 + time * 0.7));

    // Combine fireflies' impact
    float fireflyEffect = f1 + f2 + f3;

    // Translate that to a color-shifting effect
    vec3 fireflyColor = hsl2rgb(vec3(fireflyEffect * 0.3, 0.6, 0.5));

    vec3 finalColor = mix(tc.rgb, fireflyColor, d * fireflyEffect);

    gl_FragColor = vec4(finalColor, alpha);
}
