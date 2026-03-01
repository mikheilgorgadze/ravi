#include "input.h"
#include "utils.h"
#include "string.h"
#include "file_io.h"

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
        int start = min(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
        int end = max(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
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
        editor->textBuffer->selectionAnchor = editor->textBuffer->cursorByteOffset;
    }

    if (IsKeyPressedRepeat(KEY_LEFT) || IsKeyPressed(KEY_LEFT)) {
        editor->isSearchingCursor = true;
        if (editor->textBuffer->cursorByteOffset<= 0) {
            editor->textBuffer->cursorByteOffset = 0;
            if (!IsKeyDown(KEY_LEFT_SHIFT)) {
                editor->textBuffer->selectionAnchor = 0;
            }
        } else {
            int charSize = buffer_get_previous_char_size(editor->textBuffer->input, editor->textBuffer->cursorByteOffset);
            editor->textBuffer->cursorByteOffset -= charSize;

            int wordStart = buffer_get_word_start(editor->textBuffer->input, editor->textBuffer->cursorByteOffset);
            if (IsKeyDown(KEY_LEFT_CONTROL)) {
                editor->textBuffer->cursorByteOffset = wordStart;
            }
            if (!IsKeyDown(KEY_LEFT_SHIFT)) {
                editor->textBuffer->selectionAnchor = editor->textBuffer->cursorByteOffset;
            }
        }
    }

    if ((IsKeyPressedRepeat(KEY_RIGHT) || IsKeyPressed(KEY_RIGHT)) && editor->textBuffer->cursorByteOffset < (int) editor->textBuffer->size) {
        editor->isSearchingCursor = true;
        int byteSize = 0;
        GetCodepoint(&editor->textBuffer->input[editor->textBuffer->cursorByteOffset], &byteSize);

        editor->textBuffer->cursorByteOffset += byteSize;

        int wordEnd = buffer_get_word_end(editor->textBuffer->input, editor->textBuffer->cursorByteOffset, editor->textBuffer->size);
        if (IsKeyDown(KEY_LEFT_CONTROL)) {
            editor->textBuffer->cursorByteOffset = wordEnd;
        }

        if (!IsKeyDown(KEY_LEFT_SHIFT)) {
            editor->textBuffer->selectionAnchor = editor->textBuffer->cursorByteOffset;
        }
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
        editor->isSearchingCursor = true;
        int startOfCurrentLine = buffer_get_line_start(editor->textBuffer->input, editor->textBuffer->cursorByteOffset);
        int currentColumn = editor->textBuffer->cursorByteOffset - startOfCurrentLine;

        int prevLineOffset = editor->textBuffer->cursorByteOffset - currentColumn - 1;

        if (prevLineOffset < 0) return;
        int startOfPrevLine = buffer_get_line_start(editor->textBuffer->input, prevLineOffset);

        int previousLineLength = prevLineOffset - startOfPrevLine;
        if (previousLineLength >= 0) {
            int minOffset = (previousLineLength < currentColumn) ? previousLineLength : currentColumn;
            int newCursorOffset = startOfPrevLine + minOffset;
            while ( (editor->textBuffer->input[newCursorOffset] & 0xC0) == 0x80) {
                newCursorOffset--;
            }
            editor->textBuffer->cursorByteOffset = newCursorOffset;
            if (!IsKeyDown(KEY_LEFT_SHIFT)) {
                editor->textBuffer->selectionAnchor = editor->textBuffer->cursorByteOffset;
            }
        }
    }

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
        editor->isSearchingCursor = true;
        int startOfCurrentLine = buffer_get_line_start(editor->textBuffer->input, editor->textBuffer->cursorByteOffset);
        int currentColumn = editor->textBuffer->cursorByteOffset - startOfCurrentLine;

        int endOfCurrentLine = buffer_get_line_end(editor->textBuffer->input, editor->textBuffer->cursorByteOffset, editor->textBuffer->size);

        if (endOfCurrentLine >= (int) editor->textBuffer->size) return;

        int startOfNextLine =  endOfCurrentLine + 1;

        int nextLineEnd = buffer_get_line_end(editor->textBuffer->input, startOfNextLine, editor->textBuffer->size);
        int nextLineLength = nextLineEnd - startOfNextLine;
        if (nextLineLength >= 0) {
            int minOffset = (nextLineLength < currentColumn) ? nextLineLength : currentColumn;
            int newCursorOffset = startOfNextLine + minOffset;
            while ( (editor->textBuffer->input[newCursorOffset] & 0xC0) == 0x80) {
                newCursorOffset--;
            }
            editor->textBuffer->cursorByteOffset = newCursorOffset;
            if (!IsKeyDown(KEY_LEFT_SHIFT)) {
                editor->textBuffer->selectionAnchor = editor->textBuffer->cursorByteOffset;
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
        if (editor->textBuffer->cursorByteOffset != editor->textBuffer->selectionAnchor) {
            int start = min(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
            int end = max(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);

            editor_record_delete_action(editor, start, (end - start));

            buffer_delete_range(editor->textBuffer, start, end);
            editor->isUpdateNeeded = true;
        } else {
            int length = buffer_get_previous_char_size(editor->textBuffer->input, editor->textBuffer->cursorByteOffset);
            editor_record_delete_action(editor, (editor->textBuffer->cursorByteOffset - length), length);
            buffer_delete_character(editor->textBuffer);
            editor->isUpdateNeeded = true;
        }
    }

    if (IsKeyPressed(KEY_DELETE) || IsKeyPressedRepeat(KEY_DELETE)) {
        editor->isSearchingCursor = true;
        if (editor->textBuffer->cursorByteOffset != editor->textBuffer->selectionAnchor) {
            int start = min(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
            int end = max(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);

            editor_record_delete_action(editor, start, (end - start));

            buffer_delete_range(editor->textBuffer, start, end);
            editor->isUpdateNeeded = true;
        } else {
            if (editor->textBuffer->cursorByteOffset >= (int) editor->textBuffer->size) return;

            int byteSize = 0;
            GetCodepoint(&editor->textBuffer->input[editor->textBuffer->cursorByteOffset], &byteSize);

            editor_record_delete_action(editor, (editor->textBuffer->cursorByteOffset - byteSize), byteSize);

            buffer_delete_range(editor->textBuffer, editor->textBuffer->cursorByteOffset, editor->textBuffer->cursorByteOffset + byteSize);
            editor->isUpdateNeeded = true;
        }
    }

    // copy
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_C)) {
        int start = min(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
        int end = max(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);

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
        int start = min(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
        int end = max(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);

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
            editor->textBuffer->cursorByteOffset = action.offset;
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
            editor->textBuffer->cursorByteOffset = action.offset;
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
        editor->searchBuffer->cursorByteOffset = 0;
        editor->searchBuffer->input[0] = '\0';
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_A)) {
        editor->textBuffer->cursorByteOffset = 0;
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
    // get user input
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
    }
}

void input_find_next_match(editor_t *editor, int startOffset) {
    if (editor->searchBuffer->size > 0) {
        char *matchingPtr = strstr(editor->textBuffer->input + startOffset, editor->searchBuffer->input);
        if (matchingPtr != NULL) {
            int matchingOffset = matchingPtr - editor->textBuffer->input;
            editor->textBuffer->cursorByteOffset = matchingOffset;
            editor->textBuffer->selectionAnchor = matchingOffset + editor->searchBuffer->size;
            editor->isSearchingCursor = true;
        } else {
            editor->textBuffer->selectionAnchor = editor->textBuffer->cursorByteOffset;
            if (startOffset > 0) {
                input_find_next_match(editor, 0);
            }
        }
    } else {
        editor->textBuffer->selectionAnchor = editor->textBuffer->cursorByteOffset;
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
        editor->isSearchingCursor = false;
        if (isMouseOnScrollbar) {
            editor->isScrolling = true;
        } else {
            editor->isScrolling = false;
            int index = input_get_index_from_mouse(editor, GetMouseX(), GetMouseY());
            editor->textBuffer->cursorByteOffset = index;
            editor->textBuffer->selectionAnchor = index;
        }
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (editor->isScrolling) {
            float relativePos = GetMouseY() - scrollBarRec->y;
            float percentage = relativePos / scrollBarRec->height;
            float maxScroll = max((editor->totalContentHeight - editor->textBox.height), 0);
            float targetScroll = max(editor->totalContentHeight * percentage, 0);
            editor->scrollOffset = min(targetScroll, maxScroll);
        } else {
            int index = input_get_index_from_mouse(editor, GetMouseX(), GetMouseY());
            editor->textBuffer->cursorByteOffset = index;
        }
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        editor->isScrolling = false;
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
        char *input = buffer->input;
        if (input[i] == '\n' || input[i] == '\0') {
            buffer->cursorByteOffset = i;
            break;
        }

        int byteSize = 0;
        int codePoint = GetCodepoint(&input[i], &byteSize);
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

