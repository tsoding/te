#include "./la.h"
Vec2f vec2f(float x, float y)
{
    return (Vec2f) {
        .x = x,
        .y = y,
    };
}

Vec2f vec2fs(float x)
{
    return vec2f(x, x);
}

Vec2f vec2f_add(Vec2f a, Vec2f b)
{
    return vec2f(a.x + b.x, a.y + b.y);
}

Vec2f vec2f_sub(Vec2f a, Vec2f b)
{
    return vec2f(a.x - b.x, a.y - b.y);
}

Vec2f vec2f_mul(Vec2f a, Vec2f b)
{
    return vec2f(a.x * b.x, a.y * b.y);
}

Vec2f vec2f_mul3(Vec2f a, Vec2f b, Vec2f c)
{
    return vec2f_mul(vec2f_mul(a, b), c);
}

Vec2f vec2f_div(Vec2f a, Vec2f b)
{
    return vec2f(a.x / b.x, a.y / b.y);
}

//////////////////////////////

Vec2i vec2i(int x, int y)
{
    return (Vec2i) {
        .x = x,
        .y = y,
    };
}

Vec2i vec2is(int x)
{
    return vec2i(x, x);
}

Vec2i vec2i_add(Vec2i a, Vec2i b)
{
    return vec2i(a.x + b.x, a.y + b.y);
}

Vec2i vec2i_sub(Vec2i a, Vec2i b)
{
    return vec2i(a.x - b.x, a.y - b.y);
}

Vec2i vec2i_mul(Vec2i a, Vec2i b)
{
    return vec2i(a.x * b.x, a.y * b.y);
}

Vec2i vec2i_mul3(Vec2i a, Vec2i b, Vec2i c)
{
    return vec2i_mul(vec2i_mul(a, b), c);
}

Vec2i vec2i_div(Vec2i a, Vec2i b)
{
    return vec2i(a.x / b.x, a.y / b.y);
}

//////////////////////////////

Vec4f vec4f(float x, float y, float z, float w)
{
    return (Vec4f) {
        .x = x,
        .y = y,
        .z = z,
        .w = w,
    };
}

Vec4f vec4fs(float x)
{
    return vec4f(x, x, x, x);
}

Vec4f vec4f_add(Vec4f a, Vec4f b)
{
    return vec4f(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

Vec4f vec4f_sub(Vec4f a, Vec4f b)
{
    return vec4f(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

Vec4f vec4f_mul(Vec4f a, Vec4f b)
{
    return vec4f(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

Vec4f vec4f_div(Vec4f a, Vec4f b)
{
    return vec4f(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
}

float lerpf(float a, float b, float t)
{
    return a + (b - a) * t;
}
