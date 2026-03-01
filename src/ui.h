#ifndef UI_H
#define UI_H

#include "editor.h"
#define LIGHT_GRAY_CLAY {200, 200, 200, 255}
#define DARK_GRAY_CLAY  {48, 48, 48, 255}
#define COLOR_BLUE {0, 0, 255, 255}
#define COLOR_ORANGE {255, 255, 0, 255}
#define COLOR_RED {255, 0, 0, 255}

void ui_initialize(int width, int height, const char *title);
void ui_build_layout(editor_t *editor, int *cursor, bool *exitFlag, Rectangle *scrollBarRec, bool *scrollBarFound);
void ui_render(Font *fonts);

#endif
