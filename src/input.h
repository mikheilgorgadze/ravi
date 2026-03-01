#ifndef INPUT_H
#define INPUT_H

#include "editor.h"

void input_handle_keyboard(editor_t *editor);
void input_handle_normal_mode(editor_t *editor);
void input_handle_search_mode(editor_t *editor);
void input_handle_prompt_mode(editor_t *editor);
void input_zoom(editor_t *editor, float zoomAmt);
void input_scroll(editor_t *editor, float scrollAmt);
void input_find_next_match(editor_t *editor, int startOffset);
void input_handle_mouse_events(editor_t *editor, Rectangle *scrollBarRec);
int  input_get_index_from_mouse(editor_t *editor, int mouseX, int mouseY);

#endif
