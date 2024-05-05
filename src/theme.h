#ifndef THEME_H
#define THEME_H

#include "la.h"
#include "stdbool.h"

typedef struct {
    Vec4f cursor;
    Vec4f notext_cursor;
    Vec4f EOF_cursor;
    Vec4f insert_cursor;
    Vec4f emacs_cursor;
    Vec4f text;
    Vec4f background;
    Vec4f logic;
    Vec4f comment;
    Vec4f hashtag;
    Vec4f string;
    Vec4f selection;
    Vec4f search;
    Vec4f line_numbers;
    Vec4f todo;
    Vec4f fixme;
    Vec4f note;
    Vec4f bug;
    Vec4f equals;
    Vec4f not_equals;
    Vec4f exclamation;
    Vec4f equals_equals;
    Vec4f less_than;
    Vec4f greater_than;
    Vec4f arrow;
    Vec4f plus;
    Vec4f minus;
    Vec4f truee;
    Vec4f falsee;
    Vec4f open_square;
    Vec4f close_square;
    Vec4f array_content;
    Vec4f current_line_number;
    Vec4f marks;
    Vec4f fb_selection;
    Vec4f link;
    Vec4f logic_or;
    Vec4f pipe;
    Vec4f logic_and;
    Vec4f ampersand;
    Vec4f multiplication;
    Vec4f pointer;
    Vec4f modeline;
    Vec4f modeline_accent;
    Vec4f minibuffer;
    Vec4f matching_parenthesis;
    Vec4f hl_line; 
    Vec4f type; 
    Vec4f function_definition; 
    Vec4f anchor; 
    Vec4f whitespace;
    Vec4f selected_whitespaces;
    Vec4f indentation_line;
    Vec4f null;
    Vec4f code_block;
    Vec4f fringe;
    Vec4f nest1;
    Vec4f nest2;
    Vec4f nest3;
    Vec4f nest4;
    Vec4f nest5;
    Vec4f nest6;
    Vec4f fill_column;
    Vec4f open_curly;
    Vec4f close_curly;
} Theme;

#define CURRENT_THEME (currentTheme) // interpolated theme

/* #define CURRENT_THEME (themes[currentThemeIndex]) */


extern Theme themes[];
extern Theme currentTheme; // Interpolated theme
extern Theme previousTheme;
extern int currentThemeIndex;
extern int previousThemeIndex; // Index of the previous theme

extern float interpolationProgress;
extern bool theme_lerp;
extern float theme_lerp_speed;
extern float theme_lerp_treshold;



void initialize_themes();
void theme_next(int *currentThemeIndex);
void theme_previous(int *currentThemeIndex);
void update_theme_interpolation(); // Function to handle interpolation
Vec4f color_lerp(Vec4f start, Vec4f end, float t); // Function to interpolate colors
void switch_to_theme(int *currentThemeIndex, int newIndex);
void transition_color(Vec4f* color_field, Vec4f target_color, float transition_speed);


#endif // THEME_H
