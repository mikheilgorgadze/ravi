#include "editor.h"
#include "stdlib.h"
#include "string.h"
#include "utils.h"
#include "input.h"
#include <math.h>

void editor_record_delete_action(editor_t *editor, int startOffset, int length) {
    if (length <= 0) return;

    char *text = malloc(length + 1);
    memcpy(text, editor->textBuffer->input + startOffset, length);
    text[length] = '\0';
    action_t action = {
        .type = ACTION_DELETE,
        .length = length,
        .offset = startOffset,
        .text = text,
    };
    record_action(&editor->historyBuffer, action);
    free(text);
}

void editor_record_insert_action(editor_t *editor, const char *text, int length) {
    action_t action = {
        .type = ACTION_INSERT,
        .length = length,
        .offset = editor->textBuffer->cursorByteOffset,
        .text = (char *)text,
    };
    record_action(&editor->historyBuffer, action);
}

void editor_copy(editor_t *editor, int start, int end) {
    if (start != end) {
        size_t size = end - start;

        char *highlightedText = (char *)malloc(size + 1);
        memcpy(highlightedText, &editor->textBuffer->input[start], size);

        highlightedText[size] = '\0';

        SetClipboardText(highlightedText);
        free(highlightedText);
    }
}

void editor_handle_font_load(Font *currentFont, const unsigned char *fileData, int dataSize, int fontSize, int *codepoints, int codepointCount) {
    if (IsFontValid(*currentFont)) {
        UnloadFont(*currentFont);
    }
    *currentFont = LoadFontFromMemory(".ttf", fileData, dataSize, fontSize, codepoints, codepointCount);
    SetTextureFilter(currentFont->texture, TEXTURE_FILTER_BILINEAR);
}

void editor_update(editor_t* editor, Rectangle *scrollBarRec) {
    input_handle_keyboard(editor);
    input_handle_mouse_events(editor, scrollBarRec);

    editor->frameCounter++;
    
}

void editor_handle_scroll(editor_t *editor) {
    float maxScroll = editor->totalContentHeight - editor->textBox.height;
    if (maxScroll < 0) maxScroll = 0;
    if (editor->scrollOffset > maxScroll) editor->scrollOffset = maxScroll;

    if (editor->scrollOffset < 0) {
        editor->scrollOffset = 0;
    }
    if (editor->isSearchingCursor) {
        if ( editor->cursorPosition.y + editor->fontSize > (editor->scrollOffset + editor->textBox.height)) {
            editor->scrollOffset = (editor->cursorPosition.y + editor->fontSize) - editor->textBox.height;
        } else if (editor->cursorPosition.y < editor->scrollOffset) {
            editor->scrollOffset = editor->cursorPosition.y;
        }
    }
}

void editor_update_text_layout(editor_t *editor) {
    editor->textBuffer->rowList.count = 0;
    buffer_push_to_row_list(&editor->textBuffer->rowList, 0);

    char *ptr = editor->textBuffer->input;
    bool running = true;

    Vector2 layoutCursor = {.x = TEXT_OFFSET_X, .y = TEXT_OFFSET_Y};

    while (running) {
        int nextCodePoint = GetCodepointNext(ptr, &editor->codepointSize);
        Font *font = editor_get_font_for_codepoint(editor, nextCodePoint);

        if (nextCodePoint == '\0') {
            running = false; 
        }

        int charWidth = 0;
        bool isNewLine = (nextCodePoint == '\n');
        bool isTab = (nextCodePoint == '\t');

        if (nextCodePoint != '\0') {
            int index = GetGlyphIndex(*font, nextCodePoint);
            charWidth = font->glyphs[index].advanceX;
        }

        if (!isNewLine && nextCodePoint != '\0' && (layoutCursor.x + charWidth > editor->textBox.width)) {
            layoutCursor.x = TEXT_OFFSET_X;
            layoutCursor.y += editor->fontSize;

            int currentIndex = (int) (ptr - editor->textBuffer->input);
            buffer_push_to_row_list(&editor->textBuffer->rowList, currentIndex);
        }

        if (nextCodePoint == '\0') break;


        if (isNewLine) {
            layoutCursor.x = TEXT_OFFSET_X;
            layoutCursor.y += editor->fontSize;

            int nextIndex = (int) (ptr - editor->textBuffer->input) + 1;
            buffer_push_to_row_list(&editor->textBuffer->rowList, nextIndex);
        } else if (isTab) {
            int index = GetGlyphIndex(editor->font, ' ');
            int spaceWidth = editor->font.glyphs[index].advanceX;
            int advance = (4 - ((int) (layoutCursor.x / spaceWidth) % 4)) * spaceWidth;
            layoutCursor.x += advance; 
        } else {
            layoutCursor.x += charWidth;
        } 
        ptr += editor->codepointSize;
    }
    double digit_count = floor(log10(editor->textBuffer->rowList.count)) + 1;
    int charIndex = GetGlyphIndex(editor->gutterFont, '0');
    int charWidth = editor->gutterFont.glyphs[charIndex].advanceX;
    editor->gutterWidth = max(150.0, charWidth * digit_count + 20.0);

    editor->totalContentHeight = layoutCursor.y + editor->fontSize + PADDING;
}

void editor_update_cursor_position(editor_t *editor) {
    int i = 0;
    for (i = 0; i < editor->textBuffer->rowList.count; i++) {
        if (i == editor->textBuffer->rowList.count - 1) break;
        if (editor->textBuffer->cursorByteOffset < editor->textBuffer->rowList.items[i + 1]) {
            break;
        }
    }
    editor->cursorPosition.y = TEXT_OFFSET_Y + (i * editor->fontSize);

    int currentX = TEXT_OFFSET_X;

    int j = editor->textBuffer->rowList.items[i];
    int codepointSize;
    int codepoint;
    while (j < editor->textBuffer->cursorByteOffset) {
        codepoint = GetCodepoint(&editor->textBuffer->input[j], &codepointSize);
        if (codepoint == '\n') break;
        if (codepoint == '\t') {
            int index = GetGlyphIndex(editor->font, ' ');
            int spaceWidth = editor->font.glyphs[index].advanceX;
            int advance = (4 - ((int) (currentX / spaceWidth) % 4)) * spaceWidth;
            currentX += advance;
        } else {
            Font *font = editor_get_font_for_codepoint(editor, codepoint);
            int glyphIndex = GetGlyphIndex(*font, codepoint);
            currentX += font->glyphs[glyphIndex].advanceX;
        }
        j += codepointSize;
    }
    editor->cursorPosition.x = currentX;
}

Font* editor_get_font_for_codepoint(editor_t *editor, int codepoint) {
    if (codepoint >= 0x10A0 && codepoint <= 0x10FF) {
        return &editor->fallbackFont;
    }

    return &editor->font;
}

