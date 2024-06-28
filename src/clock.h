#ifndef CLOCK_H
#define CLOCK_H

#include "common.h"
#include "simple_renderer.h"

extern Vec2f clockPosition;
extern float clockScale;
extern Vec4f clockColor;

void render_clock(Simple_Renderer *sr, int hours, int minutes);
void render_digit(Simple_Renderer *sr, int digit, Vec2f position, float squareSize, Vec4f color);
void render_colon(Simple_Renderer *sr, Vec2f position, float squareSize, Vec4f color);
void init_clock();

#endif // CLOCK_H
