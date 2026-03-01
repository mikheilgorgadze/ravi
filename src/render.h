#ifndef RENDER_H
#define RENDER_H

#include "editor.h"
#include <raylib.h>

#define GUTTER_COLOR CLITERAL(Color) {35, 35, 35, 255}

// maps token types to colors
extern Color color_theme[10];

void render_draw_cursor(editor_t *editor, Vector2 position, Color color);
void render_text(editor_t *editor);
void render_gutter(editor_t *editor);
void render_search_bar_text(editor_t *editor);
void render_scroll_bar(editor_t *editor, Rectangle boundingBox, bool isFound);

#endif
