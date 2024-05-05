#include "theme.h"
#include "common.h"
#include "editor.h"
#include <stdbool.h>

int currentThemeIndex = 0;
int previousThemeIndex = 0;
float interpolationProgress;
Theme themes[10];
Theme currentTheme;
Theme previousTheme;

bool theme_lerp = true;
float theme_lerp_speed = 0.005f;
float theme_lerp_treshold = 1.0f; // 0.5 mix themes

Vec4f color_lerp(Vec4f start, Vec4f end, float t) {
  Vec4f result;
  result.x = start.x + (end.x - start.x) * t;
  result.y = start.y + (end.y - start.y) * t;
  result.z = start.z + (end.z - start.z) * t;
  result.w = start.w + (end.w - start.w) * t;
  return result;
}

// Function to smoothly transition a color field in the current theme
void transition_color(Vec4f *color_field, Vec4f target_color,
                      float transition_speed) {
  if (theme_lerp) {
    *color_field = color_lerp(*color_field, target_color, transition_speed);
  } else {
    *color_field = target_color;
  }
}

void switch_to_theme(int *currentThemeIndex, int newIndex) {
  const int themeCount = sizeof(themes) / sizeof(themes[0]);

  // Check if newIndex is valid
  if (newIndex < 0 || newIndex >= themeCount) {
    return; // Invalid index, do nothing
  }

  // Update previous theme information
  previousTheme = currentTheme;
  previousThemeIndex = *currentThemeIndex;

  // Set the new theme index
  *currentThemeIndex = newIndex;

  // Reset interpolation progress
  interpolationProgress = 0.0f;

  if (!theme_lerp) {
    // If theme lerp is disabled, set the current theme immediately
    currentTheme = themes[*currentThemeIndex];
  }
}

void theme_next(int *currentThemeIndex) {
  previousTheme = currentTheme; // Capture the current interpolated state
  previousThemeIndex = *currentThemeIndex;

  const int themeCount = sizeof(themes) / sizeof(themes[0]);
  *currentThemeIndex = (*currentThemeIndex + 1) % themeCount;

  // Check if the new index is 7; if so, skip it
  if (*currentThemeIndex == 7) {
    *currentThemeIndex = (*currentThemeIndex + 1) % themeCount;
  }

  if (!theme_lerp) {
    currentTheme = themes[*currentThemeIndex];
  }
  interpolationProgress = 0.0f; // Restart interpolation
}

void theme_previous(int *currentThemeIndex) {
  previousTheme = currentTheme; // Capture the current interpolated state
  previousThemeIndex = *currentThemeIndex;

  *currentThemeIndex -= 1;
  if (*currentThemeIndex < 0) {
    const int themeCount = sizeof(themes) / sizeof(themes[0]);
    *currentThemeIndex = themeCount - 1;
  }

  // Check if the new index is 7; if so, skip it
  if (*currentThemeIndex == 7) {
    *currentThemeIndex -= 1;
    if (*currentThemeIndex < 0) {
      const int themeCount = sizeof(themes) / sizeof(themes[0]);
      *currentThemeIndex = themeCount - 1;
    }
  }

  if (!theme_lerp) {
    currentTheme = themes[*currentThemeIndex];
  }

  interpolationProgress = 0.0f; // Restart interpolation
}

void update_theme_interpolation() {
  if (theme_lerp && interpolationProgress < theme_lerp_treshold) {
    interpolationProgress += theme_lerp_speed;
    Theme startTheme = previousTheme;
    Theme endTheme = themes[currentThemeIndex];

    // Interpolate each color component
    currentTheme.cursor = color_lerp(startTheme.cursor, endTheme.cursor, interpolationProgress);
    currentTheme.insert_cursor = color_lerp(startTheme.insert_cursor, endTheme.insert_cursor, interpolationProgress);
    currentTheme.emacs_cursor = color_lerp(startTheme.emacs_cursor, endTheme.emacs_cursor, interpolationProgress);
    currentTheme.text = color_lerp(startTheme.text, endTheme.text, interpolationProgress);
    currentTheme.background = color_lerp(startTheme.background, endTheme.background, interpolationProgress);
    currentTheme.logic = color_lerp(startTheme.logic, endTheme.logic, interpolationProgress);
    currentTheme.comment = color_lerp(startTheme.comment, endTheme.comment, interpolationProgress);
    currentTheme.hashtag = color_lerp(startTheme.hashtag, endTheme.hashtag, interpolationProgress);
    currentTheme.string = color_lerp(startTheme.string, endTheme.string, interpolationProgress);
    currentTheme.selection = color_lerp(startTheme.selection, endTheme.selection, interpolationProgress);
    currentTheme.search = color_lerp(startTheme.search, endTheme.search, interpolationProgress);
    currentTheme.line_numbers = color_lerp(startTheme.line_numbers, endTheme.line_numbers, interpolationProgress);
    currentTheme.todo = color_lerp(startTheme.todo, endTheme.todo, interpolationProgress);
    currentTheme.fixme = color_lerp(startTheme.fixme, endTheme.fixme, interpolationProgress);
    currentTheme.note = color_lerp(startTheme.note, endTheme.note, interpolationProgress);
    currentTheme.bug = color_lerp(startTheme.bug, endTheme.bug, interpolationProgress);
    currentTheme.equals = color_lerp(startTheme.equals, endTheme.equals, interpolationProgress);
    currentTheme.not_equals = color_lerp(startTheme.not_equals, endTheme.not_equals, interpolationProgress);
    currentTheme.exclamation = color_lerp(startTheme.exclamation, endTheme.exclamation, interpolationProgress);
    currentTheme.equals_equals = color_lerp(startTheme.equals_equals, endTheme.equals_equals, interpolationProgress);
    currentTheme.less_than = color_lerp(startTheme.less_than, endTheme.less_than, interpolationProgress);
    currentTheme.greater_than = color_lerp(startTheme.greater_than, endTheme.greater_than, interpolationProgress);
    currentTheme.arrow = color_lerp(startTheme.arrow, endTheme.arrow, interpolationProgress);
    currentTheme.plus = color_lerp(startTheme.plus, endTheme.plus, interpolationProgress);
    currentTheme.minus = color_lerp(startTheme.minus, endTheme.minus, interpolationProgress);
    currentTheme.truee = color_lerp(startTheme.truee, endTheme.truee, interpolationProgress);
    currentTheme.falsee = color_lerp(startTheme.falsee, endTheme.falsee, interpolationProgress);
    currentTheme.open_square = color_lerp(startTheme.open_square, endTheme.open_square, interpolationProgress);
    currentTheme.close_square = color_lerp(startTheme.close_square, endTheme.close_square, interpolationProgress);
    currentTheme.array_content = color_lerp(startTheme.array_content, endTheme.array_content, interpolationProgress);
    currentTheme.current_line_number = color_lerp(startTheme.current_line_number, endTheme.current_line_number, interpolationProgress);
    currentTheme.marks = color_lerp(startTheme.marks, endTheme.marks, interpolationProgress);
    currentTheme.fb_selection = color_lerp(startTheme.fb_selection, endTheme.fb_selection, interpolationProgress);
    currentTheme.link = color_lerp(startTheme.link, endTheme.link, interpolationProgress);
    currentTheme.logic_or = color_lerp(startTheme.logic_or, endTheme.logic_or, interpolationProgress);
    currentTheme.pipe = color_lerp(startTheme.pipe, endTheme.pipe, interpolationProgress);
    currentTheme.logic_and = color_lerp(startTheme.logic_and, endTheme.logic_and, interpolationProgress);
    currentTheme.ampersand = color_lerp(startTheme.ampersand, endTheme.ampersand, interpolationProgress);
    currentTheme.multiplication = color_lerp(startTheme.multiplication, endTheme.multiplication, interpolationProgress);
    currentTheme.pointer = color_lerp(startTheme.pointer, endTheme.pointer, interpolationProgress);
    currentTheme.modeline = color_lerp(startTheme.modeline, endTheme.modeline, interpolationProgress);
    currentTheme.modeline_accent = color_lerp(startTheme.modeline_accent, endTheme.modeline_accent, interpolationProgress);
    currentTheme.minibuffer = color_lerp(startTheme.minibuffer, endTheme.minibuffer, interpolationProgress);
    currentTheme.matching_parenthesis = color_lerp(startTheme.matching_parenthesis, endTheme.matching_parenthesis, interpolationProgress);
    currentTheme.hl_line = color_lerp(startTheme.hl_line, endTheme.hl_line, interpolationProgress);
    currentTheme.type = color_lerp(startTheme.type, endTheme.type, interpolationProgress);
    currentTheme.function_definition = color_lerp(startTheme.function_definition, endTheme.function_definition, interpolationProgress);
    currentTheme.anchor = color_lerp(startTheme.anchor, endTheme.anchor, interpolationProgress);
    currentTheme.whitespace = color_lerp(startTheme.whitespace, endTheme.whitespace, interpolationProgress);
    currentTheme.indentation_line = color_lerp(startTheme.indentation_line, endTheme.indentation_line, interpolationProgress);
    currentTheme.null = color_lerp(startTheme.null, endTheme.null, interpolationProgress);
    currentTheme.code_block = color_lerp(startTheme.code_block, endTheme.code_block, interpolationProgress);
    currentTheme.fringe = color_lerp(startTheme.fringe, endTheme.fringe, interpolationProgress);
    currentTheme.fill_column = color_lerp(startTheme.fill_column, endTheme.fill_column, interpolationProgress);
    currentTheme.open_curly = color_lerp(startTheme.open_curly, endTheme.open_curly, interpolationProgress);
    currentTheme.close_curly = color_lerp(startTheme.close_curly, endTheme.close_curly, interpolationProgress);

    if (interpolationProgress >= 1.0f) {
      interpolationProgress = 1.0f;
    }
  } else if (!theme_lerp) {
    currentTheme = themes[currentThemeIndex];
    interpolationProgress = 1.0f;
  }
}


// TODO each theme should have a name not only an index
void initialize_themes() {
  // Nature
  themes[0] = (Theme){
      .cursor = hex_to_vec4f(0x658B5FFF),
      .notext_cursor = hex_to_vec4f(0x658B5FFF),
      .EOF_cursor = hex_to_vec4f(0x658B5FFF),
      .insert_cursor = hex_to_vec4f(0x514B8EFF),
      .emacs_cursor = hex_to_vec4f(0x565663FF),
      .text = hex_to_vec4f(0xC0ACD1FF),
      .background = hex_to_vec4f(0x090909FF),
      .fringe = hex_to_vec4f(0x090909FF),
      .comment = hex_to_vec4f(0x867892FF),
      .hashtag = hex_to_vec4f(0x658B5FFF),
      .logic = hex_to_vec4f(0x658B5FFF),
      .string = hex_to_vec4f(0x4C6750FF),
      .selection = hex_to_vec4f(0x262626FF),
      .search = hex_to_vec4f(0x262626FF),
      .todo = hex_to_vec4f(0x565663FF),
      .line_numbers = hex_to_vec4f(0x171717FF),
      .current_line_number = hex_to_vec4f(0xC0ACD1FF),
      .fixme = hex_to_vec4f(0x444E46FF),
      .note = hex_to_vec4f(0x4C6750FF),
      .bug = hex_to_vec4f(0x867892FF),
      .not_equals = hex_to_vec4f(0x867892FF),
      .exclamation = hex_to_vec4f(0x4C6750FF),
      .equals = hex_to_vec4f(0xC0ACD1FF),
      .equals_equals = hex_to_vec4f(0x658B5FFF),
      .greater_than = hex_to_vec4f(0x834EB6FF),
      .less_than = hex_to_vec4f(0x834EB6FF),
      .marks = hex_to_vec4f(0x658B5FFF),
      .fb_selection = hex_to_vec4f(0x262626FF),
      .plus = hex_to_vec4f(0x658B5FFF),
      .minus = hex_to_vec4f(0x658B5FFF),
      .truee = hex_to_vec4f(0x4C6750FF),
      .falsee = hex_to_vec4f(0x867892FF),
      .arrow = hex_to_vec4f(0x834EB6FF),
      .open_curly = hex_to_vec4f(0x4C6750FF),
      .close_curly = hex_to_vec4f(0x4C6750FF),
      .open_square = hex_to_vec4f(0x514B8EFF),
      .close_square = hex_to_vec4f(0x514B8EFF),
      .array_content = hex_to_vec4f(0xC0ACD1FF),
      .link = hex_to_vec4f(0x565663FF),
      .logic_or = hex_to_vec4f(0x658B5FFF),
      .pipe = hex_to_vec4f(0x565663FF),
      .ampersand = hex_to_vec4f(0x658B5FFF),
      .logic_and = hex_to_vec4f(0x658B5FFF),
      .pointer = hex_to_vec4f(0x514B8EFF),
      .multiplication = hex_to_vec4f(0x867892FF),
      .matching_parenthesis = hex_to_vec4f(0x262626FF),
      .hl_line = hex_to_vec4f(0x070707FF),
      .type = hex_to_vec4f(0x565663FF),
      .function_definition = hex_to_vec4f(0x564F96FF),
      .anchor = hex_to_vec4f(0x564F96FF),
      .minibuffer = hex_to_vec4f(0x090909FF),
      .modeline = hex_to_vec4f(0x060606FF),
      .modeline_accent = hex_to_vec4f(0x658B5FFF),
      .whitespace = hex_to_vec4f(0x171717FF),
      .selected_whitespaces = hex_to_vec4f(0x9989A7FF),
      .indentation_line = hex_to_vec4f(0x171717FF),
      .null = hex_to_vec4f(0x564F96FF),
      .code_block = hex_to_vec4f(0x080808FF),
      .nest1 = hex_to_vec4f(0x658B5FFF),
      .nest2 = hex_to_vec4f(0x514B8EFF),
      .nest3 = hex_to_vec4f(0x658B5FFF),
      .nest4 = hex_to_vec4f(0x514B8EFF),
      .nest5 = hex_to_vec4f(0x658B5FFF),
      .nest6 = hex_to_vec4f(0x514B8EFF),
      .fill_column = hex_to_vec4f(0x262626FF),
  };
  
  // DOOM one
  themes[1] = (Theme){
      .cursor = hex_to_vec4f(0x51AFEFFF),        // #51AFEF
      .notext_cursor = hex_to_vec4f(0x51AFEFFF), // #51AFEF
      .EOF_cursor = hex_to_vec4f(0x51AFEFFF),    // #51AFEF
      .insert_cursor = hex_to_vec4f(0x51AFEFFF),
      .emacs_cursor = hex_to_vec4f(0xECBE7BFF), // #ECBE7B
      .text = hex_to_vec4f(0xBBC2CFFF),
      .background = hex_to_vec4f(0x282C34FF),
      .fringe = hex_to_vec4f(0x282C34FF),
      .comment = hex_to_vec4f(0x5B6268FF),
      .hashtag = hex_to_vec4f(0x51AFEFFF),
      .logic = hex_to_vec4f(0x51AFEFFF),
      .string = hex_to_vec4f(0x98BE65FF), // #98BE65
      .selection = hex_to_vec4f(0x42444AFF),
      .search = hex_to_vec4f(0x387AA7FF), // #387AA7
      .todo = hex_to_vec4f(0xECBE7BFF),
      .line_numbers = hex_to_vec4f(0x3F444AFF),
      .current_line_number = hex_to_vec4f(0xBBC2CFFF),
      .fixme = hex_to_vec4f(0xFF6C6BFF), // #FF6C6B
      .note = hex_to_vec4f(0x98BE65FF),
      .bug = hex_to_vec4f(0xFF6C6BFF),
      .not_equals = hex_to_vec4f(0xFF6C6BFF),
      .exclamation = hex_to_vec4f(0x51AFEFFF),
      .equals = hex_to_vec4f(0x98BE65FF),
      .equals_equals = hex_to_vec4f(0x98BE65FF),
      .greater_than = hex_to_vec4f(0x98BE65FF),
      .less_than = hex_to_vec4f(0xFF6C6BFF),
      .marks = hex_to_vec4f(0x387AA7FF),
      .fb_selection = hex_to_vec4f(0x42444AFF),
      .plus = hex_to_vec4f(0x98BE65FF),
      .minus = hex_to_vec4f(0xFF6C6BFF),
      .truee = hex_to_vec4f(0x98BE65FF),
      .falsee = hex_to_vec4f(0xFF6C6BFF),
      .arrow = hex_to_vec4f(0xBBC2CFFF),
      .open_curly = hex_to_vec4f(0x51AFEFFF),
      .close_curly = hex_to_vec4f(0x51AFEFFF),
      .open_square = hex_to_vec4f(0xBBC2CFFF),
      .close_square = hex_to_vec4f(0xBBC2CFFF),
      .array_content = hex_to_vec4f(0xA9A1E1FF),
      .link = hex_to_vec4f(0xA9A1E1FF), // #A9A1E1
      .matching_parenthesis = hex_to_vec4f(0x42444AFF),
      .type = hex_to_vec4f(0xECBE7BFF),
      .function_definition = hex_to_vec4f(0xC678DDFF), // #C678DD
      .anchor = hex_to_vec4f(0xA9A1E1FF),
      .hl_line = hex_to_vec4f(0x21242BFF), // #21242B
      .multiplication = hex_to_vec4f(0x98BE65FF),
      .pointer = hex_to_vec4f(0xA9A1E1FF),
      .logic_and = hex_to_vec4f(0x98BE65FF),
      .logic_or = hex_to_vec4f(0xFF6C6BFF),
      .ampersand = hex_to_vec4f(0x51AFEFFF),
      .pipe = hex_to_vec4f(0x98BE65FF),
      .minibuffer = hex_to_vec4f(0x21242BFF),
      .modeline = hex_to_vec4f(0x1D2026FF),
      .modeline_accent = hex_to_vec4f(0x51AFEFFF),
      .whitespace = hex_to_vec4f(0x3F444AFF),
      .selected_whitespaces = hex_to_vec4f(0x959BA5FF),
      .indentation_line = hex_to_vec4f(0x3F444AFF),
      .null = hex_to_vec4f(0xA9A1E1FF),
      .code_block = hex_to_vec4f(0x23272EFF),
      .fill_column = hex_to_vec4f(0x42444AFF),
  };

  // Dracula
  themes[2] = (Theme){
      .cursor = hex_to_vec4f(0xBD93F9FF),        // #BD93F9
      .notext_cursor = hex_to_vec4f(0xBD93F9FF), // #BD93F9
      .EOF_cursor = hex_to_vec4f(0xBD93F9FF),    // #BD93F9
      .insert_cursor = hex_to_vec4f(0xBD93F9FF),
      .emacs_cursor = hex_to_vec4f(0xF1FA8CFF), // #F1FA8C
      .text = hex_to_vec4f(0xF8F8F2FF),
      .background = hex_to_vec4f(0x282A36FF),
      .fringe = hex_to_vec4f(0x282A36FF),
      .comment = hex_to_vec4f(0x6272A4FF),
      .hashtag = hex_to_vec4f(0xBD93F9FF),
      .logic = hex_to_vec4f(0xFF79C6FF), // #FF79C6
      .string = hex_to_vec4f(0xF1FA8CFF),
      .selection = hex_to_vec4f(0x44475AFF),
      .search = hex_to_vec4f(0x8466AEFF), // #8466AE
      .todo = hex_to_vec4f(0xF1FA8CFF),
      .line_numbers = hex_to_vec4f(0x6272A4FF),
      .current_line_number = hex_to_vec4f(0xF8F8F2FF),
      .fixme = hex_to_vec4f(0xFF5555FF), // #FF5555
      .note = hex_to_vec4f(0x50FA7BFF),  // #50FA7B
      .bug = hex_to_vec4f(0xFF5555FF),
      .not_equals = hex_to_vec4f(0xFF5555FF),
      .exclamation = hex_to_vec4f(0xBD93F9FF),
      .equals = hex_to_vec4f(0x50FA7BFF),
      .equals_equals = hex_to_vec4f(0x50FA7BFF),
      .greater_than = hex_to_vec4f(0x50FA7BFF),
      .less_than = hex_to_vec4f(0xFF5555FF),
      .marks = hex_to_vec4f(0x8466AEFF),
      .fb_selection = hex_to_vec4f(0x44475AFF),
      .plus = hex_to_vec4f(0x50FA7BFF),
      .minus = hex_to_vec4f(0xFF5555FF),
      .truee = hex_to_vec4f(0x50FA7BFF),
      .falsee = hex_to_vec4f(0xFF5555FF),
      .arrow = hex_to_vec4f(0x8BE9FDFF), // #8BE9FD
      .open_curly = hex_to_vec4f(0xFF79C6FF),
      .close_curly = hex_to_vec4f(0xFF79C6FF),
      .open_square = hex_to_vec4f(0xF8F8F2FF),
      .close_square = hex_to_vec4f(0xF8F8F2FF),
      .array_content = hex_to_vec4f(0xBD93F9FF),
      .link = hex_to_vec4f(0x8BE9FDFF),
      .matching_parenthesis = hex_to_vec4f(0x44475AFF),
      .type = hex_to_vec4f(0xBD93F9FF),
      .function_definition = hex_to_vec4f(0x50FA7BFF),
      .anchor = hex_to_vec4f(0xFF79C6FF),
      .hl_line = hex_to_vec4f(0x1E2029FF), // #1E2029
      .multiplication = hex_to_vec4f(0x50FA7BFF),
      .pointer = hex_to_vec4f(0xFFC9E8FF), // #FFC9E8
      .logic_and = hex_to_vec4f(0x50FA7BFF),
      .logic_or = hex_to_vec4f(0xFF5555FF),
      .ampersand = hex_to_vec4f(0x8BE9FDFF),
      .pipe = hex_to_vec4f(0x50FA7BFF),
      .minibuffer = hex_to_vec4f(0x1E2029FF), // #1E2029
      .modeline = hex_to_vec4f(0x22232DFF),
      .modeline_accent = hex_to_vec4f(0xBD93F9FF),
      .whitespace = hex_to_vec4f(0x565761FF),
      .selected_whitespaces = hex_to_vec4f(0xC6C6C1FF),
      .indentation_line = hex_to_vec4f(0x565761FF),
      .null = hex_to_vec4f(0x8BE9FDFF),
      .code_block = hex_to_vec4f(0x23242FFF),
      .fill_column = hex_to_vec4f(0x44475AFF),
  };

  // Palenigh
  themes[3] = (Theme){
      .cursor = hex_to_vec4f(0xC792EAFF),        // #C792EA
      .notext_cursor = hex_to_vec4f(0xC792EAFF), // #C792EA
      .EOF_cursor = hex_to_vec4f(0xC792EAFF),    // #C792EA
      .insert_cursor = hex_to_vec4f(0xC792EAFF),
      .emacs_cursor = hex_to_vec4f(0xFFCB6BFF), // #FFCB6B
      .text = hex_to_vec4f(0xEEFFFFFF),
      .background = hex_to_vec4f(0x292D3EFF),
      .fringe = hex_to_vec4f(0x292D3EFF),
      .comment = hex_to_vec4f(0x676E95FF),
      .hashtag = hex_to_vec4f(0x89DDFFFF), // #89DDFF
      .logic = hex_to_vec4f(0x89DDFFFF),
      .string = hex_to_vec4f(0xC3E88DFF), // #C3E88D
      .selection = hex_to_vec4f(0x3C435EFF),
      .search = hex_to_vec4f(0x4E5579FF),
      .todo = hex_to_vec4f(0xFFCB6BFF),
      .line_numbers = hex_to_vec4f(0x676E95FF),
      .current_line_number = hex_to_vec4f(0xEEFFFFFF),
      .fixme = hex_to_vec4f(0xFF5370FF), // #FF5370
      .note = hex_to_vec4f(0xC3E88DFF),
      .bug = hex_to_vec4f(0xFF5370FF),
      .not_equals = hex_to_vec4f(0xFF5370FF),
      .exclamation = hex_to_vec4f(0x89DDFFFF),
      .equals = hex_to_vec4f(0xC3E88DFF),
      .equals_equals = hex_to_vec4f(0xC3E88DFF),
      .greater_than = hex_to_vec4f(0xC3E88DFF),
      .less_than = hex_to_vec4f(0xFF5370FF),
      .marks = hex_to_vec4f(0x4E5579FF),
      .fb_selection = hex_to_vec4f(0x3C435EFF),
      .plus = hex_to_vec4f(0xC3E88DFF),
      .minus = hex_to_vec4f(0xFF5370FF),
      .truee = hex_to_vec4f(0xC3E88DFF),
      .falsee = hex_to_vec4f(0xFF5370FF),
      .arrow = hex_to_vec4f(0xFFCB6BFF),
      .open_curly = hex_to_vec4f(0xC792EAFF),
      .close_curly = hex_to_vec4f(0xC792EAFF),
      .open_square = hex_to_vec4f(0xEEFFFFFF),
      .close_square = hex_to_vec4f(0xEEFFFFFF),
      .array_content = hex_to_vec4f(0x82AAFFFF), // #82AAFF
      .link = hex_to_vec4f(0x89DDFFFF),
      .logic_or = hex_to_vec4f(0xFF5370FF),
      .pipe = hex_to_vec4f(0xC3E88DFF),
      .ampersand = hex_to_vec4f(0x89DDFFFF),
      .logic_and = hex_to_vec4f(0xC3E88DFF),
      .pointer = hex_to_vec4f(0xF78C6CFF), // #F78C6C
      .multiplication = hex_to_vec4f(0xC3E88DFF),
      .matching_parenthesis = hex_to_vec4f(0x3C435EFF),
      .hl_line = hex_to_vec4f(0x242837FF),
      .type = hex_to_vec4f(0xC792EAFF),
      .function_definition = hex_to_vec4f(0x82AAFFFF),
      .anchor = hex_to_vec4f(0xFF5370FF),
      .minibuffer = hex_to_vec4f(0x292D3EFF),
      .modeline = hex_to_vec4f(0x232635FF),
      .modeline_accent = hex_to_vec4f(0xC792EAFF),
      .whitespace = hex_to_vec4f(0x4E5579FF),
      .selected_whitespaces = hex_to_vec4f(0xBECCCCFF),
      .indentation_line = hex_to_vec4f(0x4E5579FF),
      .null = hex_to_vec4f(0xF78C6CFF),
      .code_block = hex_to_vec4f(0x232635FF),
      .fill_column = hex_to_vec4f(0x3C435EFF),
  };

  // DOOM city lights
  themes[4] = (Theme){
      .cursor = hex_to_vec4f(0x5EC4FFFF),        // #5EC4FF
      .notext_cursor = hex_to_vec4f(0x5EC4FFFF), // #5EC4FF
      .EOF_cursor = hex_to_vec4f(0x5EC4FFFF),    // #5EC4FF
      .insert_cursor = hex_to_vec4f(0xE27E8DFF), // #E27E8D
      .emacs_cursor = hex_to_vec4f(0xEBBF83FF),  // #EBBF83
      .text = hex_to_vec4f(0xA0B3C5FF),
      .background = hex_to_vec4f(0x1D252CFF),
      .fringe = hex_to_vec4f(0x1D252CFF),
      .comment = hex_to_vec4f(0x41505EFF),
      .hashtag = hex_to_vec4f(0x5EC4FFFF),
      .logic = hex_to_vec4f(0x5EC4FFFF),
      .string = hex_to_vec4f(0x539AFCFF), // #539AFC
      .selection = hex_to_vec4f(0x28323BFF),
      .search = hex_to_vec4f(0x4189B2FF),
      .todo = hex_to_vec4f(0xEBBF83FF),
      .line_numbers = hex_to_vec4f(0x384551FF),
      .current_line_number = hex_to_vec4f(0xA0B3C5FF),
      .fixme = hex_to_vec4f(0xD95468FF), // #D95468
      .note = hex_to_vec4f(0x8BD49CFF),  // #8BD49C
      .bug = hex_to_vec4f(0xD95468FF),
      .not_equals = hex_to_vec4f(0xD95468FF),
      .exclamation = hex_to_vec4f(0x5EC4FFFF),
      .equals = hex_to_vec4f(0x8BD49CFF),
      .equals_equals = hex_to_vec4f(0x8BD49CFF),
      .greater_than = hex_to_vec4f(0x8BD49CFF),
      .less_than = hex_to_vec4f(0xD95468FF),
      .marks = hex_to_vec4f(0x4189B2FF),
      .fb_selection = hex_to_vec4f(0x28323BFF),
      .plus = hex_to_vec4f(0x8BD49CFF),
      .minus = hex_to_vec4f(0xD95468FF),
      .truee = hex_to_vec4f(0x8BD49CFF),
      .falsee = hex_to_vec4f(0xD95468FF),
      .arrow = hex_to_vec4f(0xA0B3C5FF),
      .open_curly = hex_to_vec4f(0x5EC4FFFF),
      .close_curly = hex_to_vec4f(0x5EC4FFFF),
      .open_square = hex_to_vec4f(0xA0B3C5FF),
      .close_square = hex_to_vec4f(0xA0B3C5FF),
      .array_content = hex_to_vec4f(0x539AFCFF),
      .link = hex_to_vec4f(0x539AFCFF),
      .matching_parenthesis = hex_to_vec4f(0x28323BFF),
      .type = hex_to_vec4f(0xEBBF83FF),
      .function_definition = hex_to_vec4f(0x33CED8FF), // #33CED8
      .anchor = hex_to_vec4f(0xE27E8DFF),
      .hl_line = hex_to_vec4f(0x181E24FF),
      .multiplication = hex_to_vec4f(0x8BD49CFF),
      .pointer = hex_to_vec4f(0x539AFCFF),
      .logic_and = hex_to_vec4f(0x8BD49CFF),
      .logic_or = hex_to_vec4f(0xD95468FF),
      .ampersand = hex_to_vec4f(0x5EC4FFFF),
      .pipe = hex_to_vec4f(0x8BD49CFF),
      .minibuffer = hex_to_vec4f(0x181E24FF),
      .modeline = hex_to_vec4f(0x181F25FF),
      .modeline_accent = hex_to_vec4f(0x5EC4FFFF),
      .whitespace = hex_to_vec4f(0x384551FF),
      .selected_whitespaces = hex_to_vec4f(0x808F9DFF),
      .indentation_line = hex_to_vec4f(0x384551FF),
      .null = hex_to_vec4f(0xE27E8DFF),
      .code_block = hex_to_vec4f(0x20282FFF),
      .fill_column = hex_to_vec4f(0x28323BFF),
  };

  // DOOM molokai
  themes[5] = (Theme){
      .cursor = hex_to_vec4f(0xFB2874FF),        // #FB2874
      .notext_cursor = hex_to_vec4f(0xFB2874FF), // #FB2874
      .EOF_cursor = hex_to_vec4f(0xFB2874FF),    // #FB2874
      .insert_cursor = hex_to_vec4f(0xFB2874FF),
      .emacs_cursor = hex_to_vec4f(0xE2C770FF), // #E2C770
      .text = hex_to_vec4f(0xD6D6D4FF),
      .background = hex_to_vec4f(0x1C1E1FFF),
      .fringe = hex_to_vec4f(0x1C1E1FFF),
      .comment = hex_to_vec4f(0x555556FF),
      .hashtag = hex_to_vec4f(0x9C91E4FF), // #9C91E4
      .logic = hex_to_vec4f(0xFB2874FF),
      .string = hex_to_vec4f(0xE2C770FF),
      .selection = hex_to_vec4f(0x4E4E4EFF),
      .search = hex_to_vec4f(0x9C91E4FF),
      .todo = hex_to_vec4f(0xE2C770FF),
      .line_numbers = hex_to_vec4f(0x555556FF),
      .current_line_number = hex_to_vec4f(0xCFC0C5FF),
      .fixme = hex_to_vec4f(0xE74C3CFF), // #E74C3C
      .note = hex_to_vec4f(0xB6E63EFF),  // #B6E63E
      .bug = hex_to_vec4f(0xE74C3CFF),
      .not_equals = hex_to_vec4f(0xE74C3CFF),
      .exclamation = hex_to_vec4f(0x9C91E4FF),
      .equals = hex_to_vec4f(0xB6E63EFF),
      .equals_equals = hex_to_vec4f(0xB6E63EFF),
      .greater_than = hex_to_vec4f(0xB6E63EFF),
      .less_than = hex_to_vec4f(0xE74C3CFF),
      .marks = hex_to_vec4f(0xB6E63EFF),
      .fb_selection = hex_to_vec4f(0x4E4E4EFF),
      .plus = hex_to_vec4f(0xB6E63EFF),
      .minus = hex_to_vec4f(0xE74C3CFF),
      .truee = hex_to_vec4f(0xB6E63EFF),
      .falsee = hex_to_vec4f(0xE74C3CFF),
      .arrow = hex_to_vec4f(0xD6D6D4FF),
      .open_curly = hex_to_vec4f(0xFB2874FF),
      .close_curly = hex_to_vec4f(0xFB2874FF),
      .open_square = hex_to_vec4f(0xD6D6D4FF),
      .close_square = hex_to_vec4f(0xD6D6D4FF),
      .array_content = hex_to_vec4f(0x9C91E4FF),
      .link = hex_to_vec4f(0x9C91E4FF),
      .matching_parenthesis = hex_to_vec4f(0x4E4E4EFF),
      .type = hex_to_vec4f(0x66D9EFFF),
      .function_definition = hex_to_vec4f(0xB6E63EFF),
      .anchor = hex_to_vec4f(0x9C91E4FF),
      .hl_line = hex_to_vec4f(0x222323FF),
      .multiplication = hex_to_vec4f(0xB6E63EFF),
      .pointer = hex_to_vec4f(0x9C91E4FF),
      .logic_and = hex_to_vec4f(0xB6E63EFF),
      .logic_or = hex_to_vec4f(0xE74C3CFF),
      .ampersand = hex_to_vec4f(0x9C91E4FF),
      .pipe = hex_to_vec4f(0xB6E63EFF),
      .minibuffer = hex_to_vec4f(0x222323FF),
      .modeline = hex_to_vec4f(0x2D2E2EFF),
      .modeline_accent = hex_to_vec4f(0xB6E63EFF),
      .whitespace = hex_to_vec4f(0x4E4E4EFF),
      .selected_whitespaces = hex_to_vec4f(0x808F9DFF),
      .indentation_line = hex_to_vec4f(0x4E4E4EFF),
      .null = hex_to_vec4f(0xFD971FFF),
      .code_block = hex_to_vec4f(0x2D2E2EFF),
      .fill_column = hex_to_vec4f(0x4E4E4EFF),
  };

  // SUNSET
  themes[6] = (Theme){
      .cursor = hex_to_vec4f(0xD9A173FF),        // #D9A173
      .notext_cursor = hex_to_vec4f(0xD9A173FF), // #D9A173
      .EOF_cursor = hex_to_vec4f(0xD9A173FF),    // #D9A173
      .insert_cursor = hex_to_vec4f(0xD46A7DFF), // #D46A7D
      .emacs_cursor = hex_to_vec4f(0x9A8B6AFF),  // #9A8B6A
      .text = hex_to_vec4f(0xCCCCC5FF),
      .background = hex_to_vec4f(0x0C0D12FF),
      .fringe = hex_to_vec4f(0x0C0D12FF),
      .comment = hex_to_vec4f(0x8E8E89FF),
      .hashtag = hex_to_vec4f(0xD9A173FF),
      .logic = hex_to_vec4f(0xD9A173FF),
      .string = hex_to_vec4f(0x6A7E74FF), // #6A7E74
      .selection = hex_to_vec4f(0x28292DFF),
      .search = hex_to_vec4f(0x805F44FF), // #805F44
      .todo = hex_to_vec4f(0x9A8B6AFF),
      .line_numbers = hex_to_vec4f(0x1B1B21FF),
      .current_line_number = hex_to_vec4f(0xCCCCC5FF),
      .fixme = hex_to_vec4f(0xC06873FF), // #C06873
      .note = hex_to_vec4f(0x6A7E74FF),
      .bug = hex_to_vec4f(0xC06873FF),
      .not_equals = hex_to_vec4f(0xD46A7DFF),
      .exclamation = hex_to_vec4f(0xD46A7DFF),
      .equals = hex_to_vec4f(0x6A7E74FF),
      .equals_equals = hex_to_vec4f(0x6A7E74FF),
      .greater_than = hex_to_vec4f(0x6A7E74FF),
      .less_than = hex_to_vec4f(0xC06873FF),
      .marks = hex_to_vec4f(0x805F44FF),
      .fb_selection = hex_to_vec4f(0x28292DFF),
      .plus = hex_to_vec4f(0x6A7E74FF),
      .minus = hex_to_vec4f(0xD46A7DFF),
      .truee = hex_to_vec4f(0x6A7E74FF),
      .falsee = hex_to_vec4f(0xD46A7DFF),
      .arrow = hex_to_vec4f(0xCCCCC5FF),
      .open_curly = hex_to_vec4f(0x6A7E74FF),
      .close_curly = hex_to_vec4f(0x6A7E74FF),
      .open_square = hex_to_vec4f(0xCCCCC5FF),
      .close_square = hex_to_vec4f(0xCCCCC5FF),
      .array_content = hex_to_vec4f(0xCCCCC5FF),
      .link = hex_to_vec4f(0xD9A173FF),
      .logic_or = hex_to_vec4f(0xD46A7DFF),
      .pipe = hex_to_vec4f(0x6A7E74FF),
      .ampersand = hex_to_vec4f(0x6A7E74FF),
      .logic_and = hex_to_vec4f(0x6A7E74FF),
      .pointer = hex_to_vec4f(0xD9A173FF),
      .multiplication = hex_to_vec4f(0x6A7E74FF),
      .matching_parenthesis = hex_to_vec4f(0x28292DFF),
      .hl_line = hex_to_vec4f(0x0A0B0FFF),
      .type = hex_to_vec4f(0x9A8B6AFF),
      .function_definition = hex_to_vec4f(0xE07084FF), // #E07084
      .anchor = hex_to_vec4f(0xE07084FF),
      .minibuffer = hex_to_vec4f(0x0C0D12FF),
      .modeline = hex_to_vec4f(0x08090CFF),
      .modeline_accent = hex_to_vec4f(0xD9A173FF),
      .whitespace = hex_to_vec4f(0x1B1B21FF),
      .selected_whitespaces = hex_to_vec4f(0xA3A39DFF),
      .indentation_line = hex_to_vec4f(0x28292DFF),
      .null = hex_to_vec4f(0xD46A7DFF),
      .code_block = hex_to_vec4f(0x0B0C11FF),
      .fill_column = hex_to_vec4f(0x28292DFF),
  };

  // Helix
  themes[7] = (Theme){
      .cursor = hex_to_vec4f(0x5A5977FF),        // #5A5977
      .notext_cursor = hex_to_vec4f(0x5A5977FF), // #5A5977
      .EOF_cursor = hex_to_vec4f(0x5A5977FF),    // #5A5977
      .insert_cursor = hex_to_vec4f(0x5A5977FF),
      .emacs_cursor = hex_to_vec4f(0x5A5977FF),
      .text = hex_to_vec4f(0xFFFFFFFF),
      .fringe = hex_to_vec4f(0x3B224CFF), // #3B224C
      .comment = hex_to_vec4f(0x697C81FF),
      .hashtag = hex_to_vec4f(0xDBBFEFFF), // #DBBFEF
      .logic = hex_to_vec4f(0xECCDBAFF),   // #ECCDBA
      .string = hex_to_vec4f(0xCCCCCCFF),
      .selection = hex_to_vec4f(0x540099FF), // #540099
      .search = hex_to_vec4f(0x540099FF),
      .todo = hex_to_vec4f(0x6F44F0FF),
      .line_numbers = hex_to_vec4f(0x5A5977FF),
      .current_line_number = hex_to_vec4f(0xDBBFEFFF),
      .fixme = hex_to_vec4f(0xF47868FF), // #F47868
      .note = hex_to_vec4f(0x6F44F0FF),
      .bug = hex_to_vec4f(0xF47868FF),
      .not_equals = hex_to_vec4f(0xDBBFEFFF), // #DBBFEF
      .exclamation = hex_to_vec4f(0xDBBFEFFF),
      .equals = hex_to_vec4f(0xDBBFEFFF),
      .equals_equals = hex_to_vec4f(0xDBBFEFFF),
      .greater_than = hex_to_vec4f(0xDBBFEFFF),
      .less_than = hex_to_vec4f(0xDBBFEFFF),
      .marks = hex_to_vec4f(0x540099FF),
      .fb_selection = hex_to_vec4f(0x540099FF),
      .plus = hex_to_vec4f(0xDBBFEFFF),
      .minus = hex_to_vec4f(0xDBBFEFFF),
      .truee = hex_to_vec4f(0xFFFFFFFF),
      .falsee = hex_to_vec4f(0xFFFFFFFF),
      .arrow = hex_to_vec4f(0xA4A0E8FF), // #A4A0E8
      .open_curly = hex_to_vec4f(0xDBBFEFFF),
      .close_curly = hex_to_vec4f(0xDBBFEFFF),
      .open_square = hex_to_vec4f(0xA4A0E8FF),
      .close_square = hex_to_vec4f(0xA4A0E8FF),
      .array_content = hex_to_vec4f(0xA4A0E8FF),
      .link = hex_to_vec4f(0xA4A0E8FF),
      .logic_or = hex_to_vec4f(0xDBBFEFFF),
      .pipe = hex_to_vec4f(0xDBBFEFFF),
      .ampersand = hex_to_vec4f(0xDBBFEFFF),
      .logic_and = hex_to_vec4f(0xDBBFEFFF),
      .pointer = hex_to_vec4f(0xFFFFFFFF),
      .multiplication = hex_to_vec4f(0xFFFFFFFF),
      .matching_parenthesis = hex_to_vec4f(0x6C6999FF),
      .hl_line = hex_to_vec4f(0x281733FF),
      .type = hex_to_vec4f(0xFFFFFFFF),
      .function_definition = hex_to_vec4f(0xFFFFFFFF),
      .anchor = hex_to_vec4f(0xFFFFFFFF),
      .minibuffer = hex_to_vec4f(0x3B224CFF),
      .modeline = hex_to_vec4f(0x281733FF),
      .modeline_accent = hex_to_vec4f(0x281733FF),
      .whitespace = hex_to_vec4f(0x281733FF),
      .selected_whitespaces = hex_to_vec4f(0xFFFFFFFF),
      .indentation_line = hex_to_vec4f(0x281733FF),
      .null = hex_to_vec4f(0xFFFFFFFF),
      .code_block = hex_to_vec4f(0x281733FF),
      .fill_column = hex_to_vec4f(0x540099FF), // #540099
  };

   themes[8] = (Theme){
      .cursor = hex_to_vec4f(0xD6A0D1FF),     //#D6A0D1
      .notext_cursor = hex_to_vec4f(0xD6A0D1FF), 
      .EOF_cursor = hex_to_vec4f(0xD6A0D1FF),    
      .insert_cursor = hex_to_vec4f(0xC79AF4FF),
      .emacs_cursor = hex_to_vec4f(0xDBAC66FF), 
      .text = hex_to_vec4f(0xD4D4D6FF), //#D4D4D6
      .background = hex_to_vec4f(0x14171EFF), //#14171E
      .fringe = hex_to_vec4f(0x14171EFF),
      .comment = hex_to_vec4f(0x454459FF),
      .hashtag = hex_to_vec4f(0xC79AF4FF), //#C79AF4
      .logic = hex_to_vec4f(0x9587DDFF), //#9587DD
      .string = hex_to_vec4f(0x62D2DBFF), //#62D2DB
      .selection = hex_to_vec4f(0x272C3AFF), //#272C3A
      .search = hex_to_vec4f(0x272C3AFF), 
      .todo = hex_to_vec4f(0xDBAC66FF), //#DBAC66
      .line_numbers = hex_to_vec4f(0x272C3AFF), //#272C3A
      .current_line_number = hex_to_vec4f(0xC79AF4FF),
      .fixme = hex_to_vec4f(0xE55C7AFF),  //#E55C7A
      .note = hex_to_vec4f(0x35BF88FF), //#35BF88
      .bug = hex_to_vec4f(0xE55C7AFF),
      .not_equals = hex_to_vec4f(0xE55C7AFF),
      .exclamation = hex_to_vec4f(0xE55C7AFF),
      .equals = hex_to_vec4f(0xD4D4D6FF),
      .equals_equals = hex_to_vec4f(0xD4D4D6FF),
      .greater_than = hex_to_vec4f(0xD4D4D6FF),
      .less_than = hex_to_vec4f(0xD4D4D6FF),
      .marks = hex_to_vec4f(0x272C3AFF),
      .fb_selection = hex_to_vec4f(0x272C3AFF),
      .plus = hex_to_vec4f(0xD4D4D6FF),
      .minus = hex_to_vec4f(0xD4D4D6FF),
      .truee = hex_to_vec4f(0x35BF88FF),
      .falsee = hex_to_vec4f(0xE55C7AFF),
      .arrow = hex_to_vec4f(0xD4D4D6FF),
      .open_curly = hex_to_vec4f(0x11CCB2FF),
      .close_curly = hex_to_vec4f(0x11CCB2FF),
      .open_square = hex_to_vec4f(0xD4D4D6FF),
      .close_square = hex_to_vec4f(0xD4D4D6FF),
      .array_content = hex_to_vec4f(0xD4D4D6FF),
      .link = hex_to_vec4f(0x41B0F3FF),  //#41B0F3
      .matching_parenthesis = hex_to_vec4f(0x272C3AFF),
      .type = hex_to_vec4f(0x11CCB2FF), //#11CCB2
      .function_definition = hex_to_vec4f(0xD6A0D1FF), //#D6A0D1
      .anchor = hex_to_vec4f(0x9587DDFF),
      .hl_line = hex_to_vec4f(0x202430FF), 
      .multiplication = hex_to_vec4f(0xD4D4D6FF),
      .pointer = hex_to_vec4f(0xD4D4D6FF),
      .logic_and = hex_to_vec4f(0x35BF88FF),
      .logic_or = hex_to_vec4f(0xE55C7AFF),
      .ampersand = hex_to_vec4f(0xC79AF4FF),
      .pipe = hex_to_vec4f(0x35BF88FF),
      .minibuffer = hex_to_vec4f(0x14171EFF),
      .modeline = hex_to_vec4f(0x191D26FF),
      .modeline_accent = hex_to_vec4f(0x9587DDFF),
      .whitespace = hex_to_vec4f(0x454459FF),
      .selected_whitespaces = hex_to_vec4f(0xBEBEC4FF),
      .indentation_line = hex_to_vec4f(0x272C3AFF),
      .null = hex_to_vec4f(0x41B0F3FF),
      .code_block = hex_to_vec4f(0x191D26FF),
      .fill_column = hex_to_vec4f(0x272C3AFF), //#272C3A
  };

   // doom-material-dark
   themes[9] = (Theme){
      .cursor = hex_to_vec4f(0xFFCB6BFF),
      .notext_cursor = hex_to_vec4f(0xFFCB6BFF), 
      .EOF_cursor = hex_to_vec4f(0xFFCB6BFF),    
      .insert_cursor = hex_to_vec4f(0xFFCB6BFF),
      .emacs_cursor = hex_to_vec4f(0xFFCB6BFF), 
      .text = hex_to_vec4f(0xEEFFFFFF),
      .background = hex_to_vec4f(0x212121FF),
      .fringe = hex_to_vec4f(0x212121FF),
      .comment = hex_to_vec4f(0x626262FF),
      .hashtag = hex_to_vec4f(0x89DDFFFF),
      .logic = hex_to_vec4f(0x89DDFFFF),
      .string = hex_to_vec4f(0xC3E88DFF), //#C3E88D
      .selection = hex_to_vec4f(0x406562FF),
      .search = hex_to_vec4f(0x617446FF), 
      .todo = hex_to_vec4f(0xFFCB6BFF),
      .line_numbers = hex_to_vec4f(0x585858FF),
      .current_line_number = hex_to_vec4f(0x89DDFFFF),
      .fixme = hex_to_vec4f(0xF57373FF), //FIXME
      .note = hex_to_vec4f(0xC3E88DFF), // NOTE
      .bug = hex_to_vec4f(0xF57373FF),
      .not_equals = hex_to_vec4f(0xF57373FF),
      .exclamation = hex_to_vec4f(0xF57373FF),
      .equals = hex_to_vec4f(0xC3E88DFF),
      .equals_equals = hex_to_vec4f(0xC3E88DFF),
      .greater_than = hex_to_vec4f(0xC3E88DFF),
      .less_than = hex_to_vec4f(0xF57373FF),
      .marks = hex_to_vec4f(0xF57373FF),
      .fb_selection = hex_to_vec4f(0xF57373FF),
      .plus = hex_to_vec4f(0xC3E88DFF),
      .minus = hex_to_vec4f(0xF57373FF),
      .truee = hex_to_vec4f(0xC3E88DFF),
      .falsee = hex_to_vec4f(0xF57373FF),
      .arrow = hex_to_vec4f(0xEEFFFFFF),
      .open_curly = hex_to_vec4f(0xC792EAFF),
      .close_curly = hex_to_vec4f(0xC792EAFF),
      .open_square = hex_to_vec4f(0xC792EAFF),
      .close_square = hex_to_vec4f(0xC792EAFF),
      .array_content = hex_to_vec4f(0xEEFFFFFF),
      .link = hex_to_vec4f(0x89DDFFFF),
      .matching_parenthesis = hex_to_vec4f(0x171F24FF),
      .type = hex_to_vec4f(0xC792EAFF),
      .function_definition = hex_to_vec4f(0x82AAFFFF), //#82AAFF
      .anchor = hex_to_vec4f(0xF57373FF),
      .hl_line = hex_to_vec4f(0x303030FF), 
      .multiplication = hex_to_vec4f(0xEEFFFFFF),
      .pointer = hex_to_vec4f(0xEEFFFFFF),
      .logic_and = hex_to_vec4f(0xC3E88DFF),
      .logic_or = hex_to_vec4f(0xF57373FF),
      .ampersand = hex_to_vec4f(0x82AAFFFF),
      .pipe = hex_to_vec4f(0xC3E88DFF),
      .minibuffer = hex_to_vec4f(0x212121FF),
      .modeline = hex_to_vec4f(0x303030FF),
      .modeline_accent = hex_to_vec4f(0xC792EAFF),
      .whitespace = hex_to_vec4f(0x4A4A4AFF),
      .selected_whitespaces = hex_to_vec4f(0x80CBC4FF),
      .indentation_line = hex_to_vec4f(0x4A4A4AFF),
      .null = hex_to_vec4f(0xF78C6CFF),
      .code_block = hex_to_vec4f(0x303030FF),
      .fill_column = hex_to_vec4f(0x4A4A4AFF),
  };




  // Initialize currentTheme to the first theme
  if (current_mode == HELIX) {
    currentTheme = themes[7];
  } else {
    currentTheme = themes[0];
  }
  previousThemeIndex = 0;
  currentThemeIndex = 0;
  interpolationProgress = 1.0f; // No interpolation needed at start
}
