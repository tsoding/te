// Clock
#include <stdbool.h>
#include "theme.h"
#include "clock.h"

const bool digit_patterns[10][5][3] = {
    // 0
    {
        {1, 1, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 1, 1}
    },
    // 1
    {
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1}
    },
    // 2
    {
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1}
    },
    // 3
    {
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1}
    },
    // 4
    {
        {1, 0, 1},
        {1, 0, 1},
        {1, 1, 1},
        {0, 0, 1},
        {0, 0, 1}
    },
    // 5
    {
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1}
    },
    // 6
    {
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1}
    },
    // 7
    {
        {1, 1, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1}
    },
    // 8
    {
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1}
    },
    // 9
    {
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1}
    }
};


Vec2f clockPosition;
float clockScale;
Vec4f clockColor;

void init_clock() {
    clockPosition = vec2f(1860, 31);
    clockScale = 3;
    clockColor = themes[currentThemeIndex].cursor;
}


void render_digit(Simple_Renderer *sr, int digit, Vec2f position, float squareSize, Vec4f color) {
    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 3; ++x) {
            if (digit_patterns[digit][y][x]) {
                Vec2f segment_position = {
                    position.x + x * squareSize,
                    position.y + (4 - y) * squareSize  // Flipping the Y-coordinate
                };
                simple_renderer_solid_rect(sr, segment_position, vec2f(squareSize, squareSize), color);
            }
        }
    }
}

void render_colon(Simple_Renderer *sr, Vec2f position, float squareSize, Vec4f color) {
    Vec2f top_dot_position = {position.x, position.y + 3.5 * squareSize};  // Flipped Y-coordinate
    Vec2f bottom_dot_position = {position.x, position.y + 0.5 * squareSize};  // Flipped Y-coordinate
    simple_renderer_solid_rect(sr, top_dot_position, vec2f(squareSize, squareSize), color);  // Top dot
    simple_renderer_solid_rect(sr, bottom_dot_position, vec2f(squareSize, squareSize), color);  // Bottom dot
}


/* void render_clock(Simple_Renderer *sr, int hours, int minutes) { */
/*     simple_renderer_set_shader(sr, VERTEX_SHADER_FIXED, SHADER_FOR_CURSOR); */

/*     Vec4f currentClockColor = themes[currentThemeIndex].cursor; */
/*     float digitWidth = 3 * clockScale;  // Width of each digit, assuming 3 units wide */
/*     float colonWidth = clockScale;  // Assuming colon is 1 unit wide */
/*     float spacing = clockScale;  // Spacing between elements */

/*     // Calculate total width for hours, colon, and minutes */
/*     float totalWidth = (digitWidth * 4) + colonWidth + (spacing * 3);  // 4 digits, 1 colon, 3 spaces */

/*     // Calculate starting X position based on total width to center the clock */
/*     float startX = clockPosition.x - (totalWidth / 2.0); */

/*     // Adjust positions based on startX */
/*     Vec2f firstHourDigitPos = vec2f(startX, clockPosition.y); */
/*     Vec2f secondHourDigitPos = vec2f(startX + digitWidth + spacing, clockPosition.y); */
/*     Vec2f colonPosition = vec2f(startX + (digitWidth * 2) + (spacing * 2), clockPosition.y); */
/*     Vec2f firstMinuteDigitPos = vec2f(colonPosition.x + colonWidth + spacing, clockPosition.y); */
/*     Vec2f secondMinuteDigitPos = vec2f(firstMinuteDigitPos.x + digitWidth + spacing, clockPosition.y); */

/*     // Render the digits and colon based on adjusted positions */
/*     render_digit(sr, hours / 10, firstHourDigitPos, clockScale, currentClockColor); */
/*     render_digit(sr, hours % 10, secondHourDigitPos, clockScale, currentClockColor); */
/*     render_colon(sr, colonPosition, clockScale, currentClockColor); */
/*     render_digit(sr, minutes / 10, firstMinuteDigitPos, clockScale, currentClockColor); */
/*     render_digit(sr, minutes % 10, secondMinuteDigitPos, clockScale, currentClockColor); */

/*     simple_renderer_flush(sr); */
/* } */


void render_clock(Simple_Renderer *sr, int hours, int minutes) {
    simple_renderer_set_shader(sr, VERTEX_SHADER_FIXED, SHADER_FOR_CURSOR);

    Vec4f currentClockColor = themes[currentThemeIndex].cursor;
    float digitWidth = 3 * clockScale;  // Width of each digit, assuming 3 units wide
    float colonWidth = clockScale;  // Assuming colon is 1 unit wide
    float spacing = clockScale;  // Spacing between elements
    float blockMove = 2 * clockScale;  // Define the block move distance

    // Adjust starting positions based on the presence of "1"
    float hourAdjust = (hours % 10 == 1) ? blockMove : 0;  // Adjust if second hour digit is "1"
    float minuteAdjust = (minutes / 10 == 1) ? -blockMove : 0;  // Adjust if first minute digit is "1"
    float secondMinuteAdjust = (minutes % 10 == 1 && minutes / 10 != 1) ? -blockMove : 0; // Adjust if second minute digit is "1" and first is not "1"

    // Calculate total width and starting X position to center the clock
    float totalWidth = (digitWidth * 4) + colonWidth + (spacing * 3) + hourAdjust + minuteAdjust;
    float startX = clockPosition.x - (totalWidth / 2.0);

    Vec2f firstHourDigitPos = vec2f(startX + hourAdjust, clockPosition.y); // Move first hour digit if needed
    Vec2f secondHourDigitPos = vec2f(startX + digitWidth + spacing + hourAdjust, clockPosition.y);
    Vec2f colonPosition = vec2f(startX + (digitWidth * 2) + (spacing * 2) + hourAdjust, clockPosition.y);
    Vec2f firstMinuteDigitPos = vec2f(colonPosition.x + colonWidth + spacing + minuteAdjust, clockPosition.y);
    Vec2f secondMinuteDigitPos = vec2f(firstMinuteDigitPos.x + digitWidth + spacing + secondMinuteAdjust, clockPosition.y);

    // Render the digits and colon based on adjusted positions
    render_digit(sr, hours / 10, firstHourDigitPos, clockScale, currentClockColor);
    render_digit(sr, hours % 10, secondHourDigitPos, clockScale, currentClockColor);
    render_colon(sr, colonPosition, clockScale, currentClockColor);
    render_digit(sr, minutes / 10, firstMinuteDigitPos, clockScale, currentClockColor);
    render_digit(sr, minutes % 10, secondMinuteDigitPos, clockScale, currentClockColor);

    simple_renderer_flush(sr);
}


