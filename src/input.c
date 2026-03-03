#include "input.h"
#include "buffer.h"
#include "utils.h"
#include "string.h"
#include "file_io.h"

#include "stdlib.h"
#include <raylib.h>

void input_handle_keyboard(editor_t *editor) {
    switch (editor->editorMode) {
        case MODE_NORMAL:
            input_handle_normal_mode(editor);
        break;
        case MODE_SEARCH:
            input_handle_search_mode(editor);
        break;
        case MODE_PROMPT:
            input_handle_prompt_mode(editor);
        break;
    }
}
void input_handle_normal_mode(editor_t *editor) {

    // get user input
    int key = GetCharPressed();
    while (key > 0) {
        int start = min(editor->textBuffer->gapStart, editor->textBuffer->selectionAnchor);
        int end = max(editor->textBuffer->gapStart, editor->textBuffer->selectionAnchor);
        if (start != end) {
            editor_record_delete_action(editor, start, (end - start));
            buffer_delete_range(editor->textBuffer, start, end);
        }

        int byteSize = 0;
        const char *utf8Symbol = buffer_codepoint_to_utf8(key, &byteSize);
        editor_record_insert_action(editor, utf8Symbol, byteSize);

        buffer_insert_bytes(editor->textBuffer, utf8Symbol, byteSize);
        editor->isUpdateNeeded = true;
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        editor->isSearchingCursor = true;
        editor->textBuffer->selectionAnchor = editor->textBuffer->gapStart;
    }

    if (IsKeyPressedRepeat(KEY_LEFT) || IsKeyPressed(KEY_LEFT)) {
        editor->isSearchingCursor = true;
        if (editor->textBuffer->gapStart<= 0) {
            buffer_move_gap(editor->textBuffer, 0);
            if (!IsKeyDown(KEY_LEFT_SHIFT)) {
                editor->textBuffer->selectionAnchor = 0;
            }
        } else {
            int targetIndex = editor->textBuffer->gapStart;
            int charSize = buffer_get_previous_char_size(editor->textBuffer, editor->textBuffer->gapStart);
            targetIndex -= charSize;

            if (IsKeyDown(KEY_LEFT_CONTROL)) {
                int wordStart = buffer_get_word_start(editor->textBuffer, editor->textBuffer->gapStart);
                targetIndex = wordStart;
            }
            buffer_move_gap(editor->textBuffer, targetIndex);

            if (!IsKeyDown(KEY_LEFT_SHIFT)) {
                editor->textBuffer->selectionAnchor = editor->textBuffer->gapStart;
            }
        }
    }

    if ((IsKeyPressedRepeat(KEY_RIGHT) || IsKeyPressed(KEY_RIGHT)) && editor->textBuffer->gapStart < (int) editor->textBuffer->size) {
        int targetIndex = editor->textBuffer->gapStart;
        editor->isSearchingCursor = true;
        int byteSize = 0;
        GetCodepoint(&editor->textBuffer->input[editor->textBuffer->gapEnd], &byteSize);

        targetIndex += byteSize;

        if (IsKeyDown(KEY_LEFT_CONTROL)) {
            int wordEnd = buffer_get_word_end(editor->textBuffer, editor->textBuffer->gapStart);
            targetIndex = wordEnd;
        }
        buffer_move_gap(editor->textBuffer, targetIndex);

        if (!IsKeyDown(KEY_LEFT_SHIFT)) {
            editor->textBuffer->selectionAnchor = editor->textBuffer->gapStart;
        }
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
        editor->isSearchingCursor = true;
        int startOfCurrentLine = buffer_get_line_start(editor->textBuffer, editor->textBuffer->gapStart);
        int currentColumn = editor->textBuffer->gapStart - startOfCurrentLine;

        int prevLineOffset = editor->textBuffer->gapStart - currentColumn - 1;

        if (prevLineOffset < 0) return;
        int startOfPrevLine = buffer_get_line_start(editor->textBuffer, prevLineOffset);

        int previousLineLength = prevLineOffset - startOfPrevLine;
        if (previousLineLength >= 0) {
            int minOffset = (previousLineLength < currentColumn) ? previousLineLength : currentColumn;
            int newCursorOffset = startOfPrevLine + minOffset;
            while ( (buffer_get_char_at(editor->textBuffer, newCursorOffset) & 0xC0) == 0x80) {
                newCursorOffset--;
            }

            buffer_move_gap(editor->textBuffer, newCursorOffset);
            if (!IsKeyDown(KEY_LEFT_SHIFT)) {
                editor->textBuffer->selectionAnchor = editor->textBuffer->gapStart;
            }
        }
    }

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
        editor->isSearchingCursor = true;
        int startOfCurrentLine = buffer_get_line_start(editor->textBuffer, editor->textBuffer->gapStart);
        int currentColumn = editor->textBuffer->gapStart - startOfCurrentLine;

        int endOfCurrentLine = buffer_get_line_end(editor->textBuffer, editor->textBuffer->gapStart);

        if (endOfCurrentLine >= (int) editor->textBuffer->size) return;

        int startOfNextLine =  endOfCurrentLine + 1;

        int nextLineEnd = buffer_get_line_end(editor->textBuffer, startOfNextLine);
        int nextLineLength = nextLineEnd - startOfNextLine;
        if (nextLineLength >= 0) {
            int minOffset = (nextLineLength < currentColumn) ? nextLineLength : currentColumn;
            int newCursorOffset = startOfNextLine + minOffset;
            while ( (buffer_get_char_at(editor->textBuffer, newCursorOffset) & 0xC0) == 0x80) {
                newCursorOffset--;
            }
            buffer_move_gap(editor->textBuffer, newCursorOffset);
            if (!IsKeyDown(KEY_LEFT_SHIFT)) {
                editor->textBuffer->selectionAnchor = editor->textBuffer->gapStart;
            }
        }
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressedRepeat(KEY_ENTER)) {
        editor->isSearchingCursor = true;
        editor_record_insert_action(editor, "\n", 1);
        buffer_insert_character(editor->textBuffer, '\n');
        editor->isUpdateNeeded = true;
    }

    if (IsKeyPressed(KEY_TAB) || IsKeyPressedRepeat(KEY_TAB)) {
        editor->isSearchingCursor = true;
        editor_record_insert_action(editor, "\t", 1);
        buffer_insert_character(editor->textBuffer, '\t');
        editor->isUpdateNeeded = true;
    }

    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        editor->isSearchingCursor = true;
        if (editor->textBuffer->gapStart != editor->textBuffer->selectionAnchor) {
            int start = min(editor->textBuffer->gapStart, editor->textBuffer->selectionAnchor);
            int end = max(editor->textBuffer->gapStart, editor->textBuffer->selectionAnchor);

            editor_record_delete_action(editor, start, (end - start));

            buffer_delete_range(editor->textBuffer, start, end);
            editor->isUpdateNeeded = true;
        } else {
            int length = buffer_get_previous_char_size(editor->textBuffer, editor->textBuffer->gapStart);
            editor_record_delete_action(editor, (editor->textBuffer->gapStart - length), length);
            buffer_delete_character(editor->textBuffer);
            editor->isUpdateNeeded = true;
        }
    }

    if (IsKeyPressed(KEY_DELETE) || IsKeyPressedRepeat(KEY_DELETE)) {
        editor->isSearchingCursor = true;
        if (editor->textBuffer->gapStart != editor->textBuffer->selectionAnchor) {
            int start = min(editor->textBuffer->gapStart, editor->textBuffer->selectionAnchor);
            int end = max(editor->textBuffer->gapStart, editor->textBuffer->selectionAnchor);

            editor_record_delete_action(editor, start, (end - start));

            buffer_delete_range(editor->textBuffer, start, end);
            editor->isUpdateNeeded = true;
        } else {
            if (editor->textBuffer->gapStart >= (int) editor->textBuffer->size) return;

            int byteSize = 0;
            GetCodepoint(&editor->textBuffer->input[editor->textBuffer->gapStart], &byteSize);

            editor_record_delete_action(editor, (editor->textBuffer->gapStart - byteSize), byteSize);

            buffer_delete_range(editor->textBuffer, editor->textBuffer->gapStart, editor->textBuffer->gapStart + byteSize);
            editor->isUpdateNeeded = true;
        }
    }

    // copy
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_C)) {
        int start = min(editor->textBuffer->gapStart, editor->textBuffer->selectionAnchor);
        int end = max(editor->textBuffer->gapStart, editor->textBuffer->selectionAnchor);

        editor_copy(editor, start, end);
    }

    // paste
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V)) {
        const char *clipboardText = GetClipboardText();

        if (clipboardText != NULL) {
            size_t pastedSize = strlen(clipboardText);
            editor_record_insert_action(editor, clipboardText, pastedSize);
            buffer_insert_bytes(editor->textBuffer, clipboardText, pastedSize);
            editor->isUpdateNeeded = true;
        }
    }

    // cut
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_X)) {
        int start = min(editor->textBuffer->gapStart, editor->textBuffer->selectionAnchor);
        int end = max(editor->textBuffer->gapStart, editor->textBuffer->selectionAnchor);

        editor_copy(editor, start, end);

        if (start != end) {
            editor_record_delete_action(editor, start, (end - start));
            buffer_delete_range(editor->textBuffer, start, end);
            editor->isUpdateNeeded = true;
        }
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_Z)) {
        action_t action = {0};
        if (undo(&editor->historyBuffer, &action)) {
            buffer_move_gap(editor->textBuffer, action.offset);
            editor->textBuffer->selectionAnchor = action.offset;
            switch (action.type) {
                case ACTION_INSERT:
                    buffer_delete_range(editor->textBuffer, action.offset, action.offset + action.length);
                break;
                case ACTION_DELETE:
                    buffer_insert_bytes(editor->textBuffer, action.text, action.length);
                break;
            }
            editor->isUpdateNeeded = true;
        }
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_Y)) {
        action_t action = {0};
        if (redo(&editor->historyBuffer, &action)) {
            buffer_move_gap(editor->textBuffer, action.offset);
            editor->textBuffer->selectionAnchor = action.offset;
            switch (action.type) {
                case ACTION_INSERT:
                    buffer_insert_bytes(editor->textBuffer, action.text, action.length);
                break;
                case ACTION_DELETE:
                    buffer_delete_range(editor->textBuffer, action.offset, action.offset + action.length);
                break;
            }
            editor->isUpdateNeeded = true;
        }
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_F)) {

        editor->editorMode = MODE_SEARCH;
        editor->searchBuffer->size = 0;
        editor->searchBuffer->gapStart = 0;
        editor->searchBuffer->gapEnd = editor->searchBuffer->capacity;
        editor->searchBuffer->selectionAnchor = 0;
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_A)) {
        buffer_move_gap(editor->textBuffer, 0);
        editor->textBuffer->selectionAnchor = editor->textBuffer->size;
    }
    
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) {
        filio_handle_file_save(editor);
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) {
        fileio_handle_file_open(editor);
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_EQUAL)) {
        input_zoom(editor, 2.0);
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_MINUS)) {
        input_zoom(editor, -2.0);
    }

}
void input_handle_search_mode(editor_t *editor) {
    int key = GetCharPressed();
    while (key > 0) {
        int byteSize = 0;
        const char *utf8Symbol = buffer_codepoint_to_utf8(key, &byteSize);
        buffer_insert_bytes(editor->searchBuffer, utf8Symbol, byteSize);
        input_find_next_match(editor, 0);
        editor->isUpdateNeeded = true;
        key = GetCharPressed();
    }


    if (IsKeyPressed(KEY_ESCAPE)) {
        editor->editorMode = MODE_NORMAL;
    }

    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        buffer_delete_character(editor->searchBuffer);
        input_find_next_match(editor, 0);
        editor->isUpdateNeeded = true;
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressedRepeat(KEY_ENTER)) {
        input_find_next_match(editor, editor->textBuffer->selectionAnchor);
    }
}

void input_handle_prompt_mode(editor_t *editor) {
    if(IsKeyPressed(KEY_ESCAPE)) {
        editor->editorMode = MODE_NORMAL;
    }
}


void input_zoom(editor_t *editor, float zoomAmt) {
    if (zoomAmt!= 0) {
        int oldFontSize = editor->fontSize;
        int sizeIncrement = editor->fontSize + (int)(zoomAmt * 3);
        int newFontSize = min(max(40, sizeIncrement), 120);
        editor->fontSize = newFontSize;
        float scale = 1.0 * newFontSize / oldFontSize;

        if (newFontSize != oldFontSize) {
            editor->fontsNeedReload = true;
            editor->isUpdateNeeded = true;
            editor->scrollOffset *= scale;
            editor->isSearchingCursor = true;
        }
    }
}

void input_scroll(editor_t *editor, float scrollAmt) {
    if (scrollAmt != 0) {
        editor->scrollOffset -= scrollAmt * editor->fontSize;
        editor->isSearchingCursor = false;
        editor->isUpdateNeeded = true;
    }
}

void input_find_next_match(editor_t *editor, int startOffset) {
    if (editor->searchBuffer->size > 0) {
        char *query = buffer_get_text(editor->searchBuffer, &editor->frameArena);
        int matchingOffset = buffer_find_text(editor->textBuffer, query, startOffset);
        if (matchingOffset >= 0) {
            buffer_move_gap(editor->textBuffer, matchingOffset);
            editor->textBuffer->selectionAnchor = matchingOffset + editor->searchBuffer->size;
            editor->isSearchingCursor = true;
        } else {
            editor->textBuffer->selectionAnchor = editor->textBuffer->gapStart;
            if (startOffset > 0) {
                input_find_next_match(editor, 0);
            }
        }
    } else {
        editor->textBuffer->selectionAnchor = editor->textBuffer->gapStart;
    }
}

void input_handle_mouse_events(editor_t *editor, Rectangle *scrollBarRec) {
    bool isMouseOnScrollbar = CheckCollisionPointRec(GetMousePosition(), *scrollBarRec);

    if (CheckCollisionPointRec(GetMousePosition(), editor->textBox)) {
        editor->mouseOnText = true;
    } else {
        editor->mouseOnText = false;
    }

    float mouseWheelMove = GetMouseWheelMove();
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        input_zoom(editor, mouseWheelMove);
    } else {
        input_scroll(editor, mouseWheelMove);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        double currentTime = GetTime();
        editor->isSearchingCursor = false;

        if (isMouseOnScrollbar) {
            editor->isScrolling = true;
        } else {
            editor->isScrolling = false;
            int index = input_get_index_from_mouse(editor, GetMouseX(), GetMouseY());

            // handle double click on word
            if (currentTime - editor->lastClickTime < 0.3) {
                editor->clickCount++;
                if (editor->clickCount > 3) editor->clickCount = 1;
            } else {
                editor->clickCount = 1;
            }

            if (editor->clickCount == 1) {
                buffer_move_gap(editor->textBuffer, index);
                editor->textBuffer->selectionAnchor = index;
                editor->isAutoSelecting = false;
            } else if (editor->clickCount == 2) {
                int wordStart = buffer_get_word_start(editor->textBuffer, index);
                int wordEnd   = buffer_get_word_end(editor->textBuffer, index);

                buffer_move_gap(editor->textBuffer, wordStart);
                editor->textBuffer->selectionAnchor = wordEnd;
                editor->isAutoSelecting = true;
            } else if (editor->clickCount == 3){
                int lineStart = buffer_get_line_start(editor->textBuffer, index);
                int lineEnd   = buffer_get_line_end(editor->textBuffer, index);

                buffer_move_gap(editor->textBuffer, lineStart);
                editor->textBuffer->selectionAnchor = lineEnd;
                editor->isAutoSelecting = true;
            }
            editor->lastClickTime = currentTime;
        }

    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (editor->isScrolling) {
            float relativePos = GetMouseY() - scrollBarRec->y;
            float percentage = relativePos / scrollBarRec->height;
            float maxScroll = max((editor->totalContentHeight - editor->textBox.height), 0);
            float targetScroll = max(editor->totalContentHeight * percentage, 0);
            float newScroll = min(targetScroll, maxScroll);

            if (editor->scrollOffset != newScroll) {
                editor->scrollOffset = newScroll;
                editor->isUpdateNeeded = true;
            }

        } else if (!editor->isAutoSelecting) {
            int index = input_get_index_from_mouse(editor, GetMouseX(), GetMouseY());
            buffer_move_gap(editor->textBuffer, index);
        }
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        editor->isScrolling = false;
        editor->isAutoSelecting = false;
    }
}

int input_get_index_from_mouse(editor_t *editor, int mouseX, int mouseY) {
    text_buffer_t *buffer = editor->textBuffer;

    int targetRow = (mouseY - (editor->textBox.y + TEXT_OFFSET_Y) + (int)editor->scrollOffset) / editor->fontSize;
    targetRow = max(targetRow, 0);

    if (targetRow >= buffer->rowList.count) targetRow = buffer->rowList.count - 1;

    int start = buffer->rowList.items[targetRow];
    int end   = (targetRow < buffer->rowList.count - 1) ? buffer->rowList.items[targetRow + 1] : buffer->size;
    int currentPixelWidth = editor->textBox.x + TEXT_OFFSET_X;

    int i = start;
    while (i < end) {
        if (buffer_get_char_at(buffer, i) == '\n' || buffer_get_char_at(buffer, i) == '\0') {
            return i;
        }

        char temp[5] = {0};
        buffer_read_utf8_sequence(buffer, i, temp);

        int byteSize = 0;
        int codePoint = GetCodepoint(temp, &byteSize);
        Font *font = editor_get_font_for_codepoint(editor, codePoint);

        int charWidth = 0;

        if (codePoint == '\t') {
            int index = GetGlyphIndex(editor->font, ' ');
            int spaceWidth = editor->font.glyphs[index].advanceX;
            charWidth = (4 - ((int)((currentPixelWidth - (editor->textBox.x)) / spaceWidth) % 4)) * spaceWidth;
        } else {
            int index = GetGlyphIndex(*font, codePoint);
            charWidth = font->glyphs[index].advanceX;
        }

        if (mouseX < currentPixelWidth + (charWidth / 2)) {
            return i;
        }

        currentPixelWidth += charWidth;
        i+=byteSize;
    }

    return i;
}

