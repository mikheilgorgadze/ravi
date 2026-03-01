#include "render.h"
#include "utils.h"

Color color_theme[10] = {
    RED,
    DARKGREEN,
    GREEN,
    ORANGE,
    PURPLE,
    PURPLE,
    GRAY,
};

void render_draw_cursor(editor_t *editor, Vector2 position, Color color) {
    Rectangle cursor = {
        .x = position.x, 
        .y = position.y, 
        .width = 3, 
        .height = editor->fontSize
    };

    if (editor->frameCounter/20 % 2 == 0) {
        DrawRectangleRec(cursor, color);
    }
}

void render_text(editor_t *editor) {
    text_buffer_t *buffer = editor->textBuffer;

    Vector2 cursorScreenPosition = (Vector2) {
        .x = editor->cursorPosition.x + editor->textBox.x,
        .y = editor->cursorPosition.y + editor->textBox.y - editor->scrollOffset
    };

    if (editor->editorMode == MODE_NORMAL) {
        render_draw_cursor(editor, cursorScreenPosition, WHITE);
    }

    Color currentColor = WHITE;


    BeginScissorMode((int)editor->textBox.x, (int)editor->textBox.y, (int)editor->textBox.width, (int)editor->textBox.height);

    int currentTokenIndex = 0;
    for (int i = 0; i < editor->textBuffer->rowList.count; i++) {
        float rowY = editor->textBox.y + TEXT_OFFSET_Y + (i * editor->fontSize) - editor->scrollOffset;
        if (rowY < editor->textBox.y - editor->fontSize || rowY > editor->textBox.y + editor->textBox.height) {
            continue;
        }

        int start = buffer->rowList.items[i];
        int end = (i < buffer->rowList.count - 1) ? buffer->rowList.items[i+1] : buffer->size;

        float currentX = TEXT_OFFSET_X;

        int highlightStart = min(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
        int highlightEnd = max(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
        while (start < end) {
            char *ptr = buffer->input + start;
            int nextCodePoint = GetCodepointNext(ptr, &editor->codepointSize);
            Font *font = editor_get_font_for_codepoint(editor, nextCodePoint);

            if (nextCodePoint == '\n') {
                start += editor->codepointSize;
                continue;
            }

            Vector2 charPosition = (Vector2) {
                .x = currentX + editor->textBox.x,
                .y = rowY
            };

            int index = GetGlyphIndex(*font, nextCodePoint);
            int charWidth = font->glyphs[index].advanceX;

            if (start >= highlightStart && start < highlightEnd) {
                Rectangle highilightRec = (Rectangle) {
                    .x = charPosition.x,
                    .y = charPosition.y,
                    .width = charWidth,
                    .height = editor->fontSize
                };
                DrawRectangleRec(highilightRec, ORANGE);
            }

            if (nextCodePoint == '\t') {
                int index = GetGlyphIndex(editor->font, ' ');
                int spaceWidth = editor->font.glyphs[index].advanceX;
                int advance = (4 - ((int) (currentX / spaceWidth) % 4)) * spaceWidth;
                currentX += advance;
            } else {    
                while (currentTokenIndex < editor->syntaxTokens.count && start >= editor->syntaxTokens.items[currentTokenIndex].end) {
                    currentTokenIndex++;
                }
                currentColor = WHITE;
                if (currentTokenIndex < editor->syntaxTokens.count && start >= editor->syntaxTokens.items[currentTokenIndex].start) {
                    currentColor = color_theme[editor->syntaxTokens.items[currentTokenIndex].keyword.token_type];
                }
                DrawTextCodepoint(*font, nextCodePoint, charPosition, editor->fontSize, currentColor);
                currentX += charWidth;
            }

            start += editor->codepointSize;
        }
    }
    EndScissorMode();

    render_gutter(editor);
}

void render_gutter(editor_t *editor) {
    DrawRectangleRec(editor->gutter, GUTTER_COLOR);

    text_buffer_t *buffer = editor->textBuffer;
    Vector2 gutterPos = {0};

    BeginScissorMode(editor->gutter.x, editor->gutter.y, editor->gutter.width, editor->gutter.height);
    int lineNumber = 1;
    for (int i = 0; i < buffer->rowList.count; i++) {
        if (i > 0 && buffer->input[buffer->rowList.items[i] - 1] == '\n') {
           lineNumber ++; 
        }

        float y = editor->gutter.y + TEXT_OFFSET_Y + (i * editor->fontSize) - editor->scrollOffset;
        if (y < editor->gutter.y - editor->fontSize || y > editor->gutter.y + editor->gutter.height) {
            continue;
        }

        if (i == 0 || buffer->input[buffer->rowList.items[i] - 1] == '\n') {
            gutterPos.y = y;
            Vector2 pos = MeasureTextEx(editor->gutterFont, TextFormat("%d", lineNumber), editor->fontSize, 1);
            gutterPos.x = editor->gutterWidth - pos.x - 10;
            DrawTextEx(editor->gutterFont, TextFormat("%d", lineNumber), gutterPos, editor->fontSize, 1, LIGHTGRAY);
        }

    }
    EndScissorMode();
}

void render_search_bar_text(editor_t *editor) {
    DrawRectangleRec(editor->searchBox, LIGHTGRAY);

    int currentX = editor->searchBox.x;
    int i = 0;

    BeginScissorMode((int)editor->searchBox.x, (int)editor->searchBox.y, (int)editor->searchBox.width, (int)editor->searchBox.height);
    while (true) {
        char *ptr = editor->searchBuffer->input + i;
        if (*ptr == '\0') break;
        int nextCodePoint = GetCodepointNext(ptr, &editor->codepointSize);
        Font *font = editor_get_font_for_codepoint(editor, nextCodePoint);

        Vector2 position = (Vector2) {
            .x = currentX,
            .y = editor->searchBox.y,
        };

        DrawTextCodepoint(*font, nextCodePoint, position, editor->fontSize, BLACK);

        int index = GetGlyphIndex(*font, nextCodePoint);
        currentX += font->glyphs[index].advanceX;

        i += editor->codepointSize;
    }
    EndScissorMode();

    Vector2 cursorPosition = (Vector2) {
        .x = currentX,
        .y = editor->searchBox.y,
    };

    render_draw_cursor(editor, cursorPosition, BLACK);
}

void render_scroll_bar(editor_t *editor, Rectangle boundingBox, bool isFound) {
    Rectangle scrollBg = {
        .x = boundingBox.x, 
        .y = boundingBox.y, 
        .width = boundingBox.width, 
        .height = boundingBox.height
    };
    DrawRectangleRec(scrollBg, GUTTER_COLOR);

    if (!isFound) return;

    if (editor->totalContentHeight > editor->textBox.height) {
        float height =max((editor->textBox.height / editor->totalContentHeight) * boundingBox.height, 10);
        Rectangle rec = (Rectangle) {
            .height = height,
            .width = boundingBox.width,
            .x = boundingBox.x,
            .y = boundingBox.y + (editor->scrollOffset / editor->totalContentHeight * boundingBox.height)
        };

        DrawRectangleRounded(rec, 0.5, 1, WHITE);
    }
}

