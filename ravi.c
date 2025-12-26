#include <raylib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define DARK_GRAY CLITERAL(Color) {48, 48, 48, 48}
#define MAX_INPUT_CHAR 100000
#define FONT_SIZE 40
#define PADDING 40

#define FONT_SANS "resources/fonts/NotoSansGeorgian-Regular.ttf"
#define FONT_MONO "resources/fonts/Everson Mono.ttf"

typedef struct {
    char input[MAX_INPUT_CHAR + 1];
    int size;
    int cursorByteOffset;
    Font font;
} TextBuffer;

typedef struct {
    const char* title;
    const int width;
    const int height;
    int codepointSize;
    int frameCounter;
    float scrollOffset;
    float cursorLayoutY;
    float totalContentHeight;
    bool isSearchingCursor;
    bool mouseOnText;
    Vector2 cursorPosition;
    TextBuffer *textBuffer;
    Rectangle textBox;
} Editor;

void InsertCharacter(TextBuffer *buffer, int key);
void DeleteCharacter(TextBuffer *buffer);
void RenderText(Editor *editor);
void UpdateEditor(Editor *editor);
void HandleScroll(Editor *editor);
void HandleArrowKeys(Editor *editor);
size_t string_len_utf8(const char *str);

int main(void) {
    const char * activeFontName = FONT_SANS;

    TextBuffer textBuffer = {
        .input = "\0",
        .cursorByteOffset = 0,
        .size = 0,
    };

    Editor editor = {
        .title = "Ravi Editor",
        .width = 1024,
        .height = 1024,
        .codepointSize = 0,
        .frameCounter = 0,
        .scrollOffset = 0.0,
        .cursorLayoutY = 0.0,
        .isSearchingCursor = false,
        .mouseOnText = false,
        .textBuffer = &textBuffer,
        .textBox = {
            .x = PADDING,
            .y = PADDING,
        },
    };
    editor.textBox.width = editor.width - PADDING * 2;
    editor.textBox.height = editor.height - PADDING * 2;

    int codepoints[512];
    int codepintCount = 0;

    //add ascii unicode character range to codepoints
    for (int i = 32; i < 127; i++) codepoints[codepintCount++] = i;

    //add georgian unicode character range to codepoints
    for (int i = 0x10A0; i < 0x10FF; i++) codepoints[codepintCount++] = i;


    SetTargetFPS(60);
    InitWindow(editor.width, editor.height, editor.title);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    textBuffer.font = LoadFontEx(activeFontName, FONT_SIZE, codepoints, codepintCount);
    SetTextureFilter(textBuffer.font.texture, TEXTURE_FILTER_BILINEAR);

    while(!WindowShouldClose()) {
        UpdateEditor(&editor);

        BeginDrawing();

        ClearBackground(DARK_GRAY);
        DrawRectangleRec(editor.textBox, LIGHTGRAY);
        RenderText(&editor);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}

void UpdateEditor(Editor* editor) {
    HandleScroll(editor);

    if (IsWindowResized()) {
        editor->textBox.width = GetScreenWidth() - PADDING * 2;
        editor->textBox.height = GetScreenHeight() - PADDING * 2;
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_F)) {
        MaximizeWindow();
    }

    HandleArrowKeys(editor);

    if (IsKeyPressed(KEY_ENTER)) {
        editor->isSearchingCursor = true;
        InsertCharacter(editor->textBuffer, '\n');
    }

    if (CheckCollisionPointRec(GetMousePosition(), editor->textBox)) {
        editor->mouseOnText = true;
    } else {
        editor->mouseOnText = false;
    }

    if (GetMouseWheelMove() != 0) {
        editor->scrollOffset -= GetMouseWheelMove() * FONT_SIZE;
        editor->isSearchingCursor = false;
    }

    if (editor->mouseOnText) {
        editor->frameCounter++;
        SetMouseCursor(MOUSE_CURSOR_IBEAM);
        int key = GetCharPressed();

        while (key > 0) {
            InsertCharacter(editor->textBuffer, key);
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            editor->isSearchingCursor = true;
            DeleteCharacter(editor->textBuffer);
        }

    } else {
        editor->frameCounter = 0;
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }
}

void InsertCharacter(TextBuffer *buffer, int key) {
    int byteSize = 0;
    if (buffer->size + 4 < MAX_INPUT_CHAR) {
        const char *utf8Symbol = CodepointToUTF8(key, &byteSize);
        //printf("Codepoint: %s\n", utf8Symbol);
        memmove(
            buffer->input + buffer->cursorByteOffset + byteSize,
            buffer->input + buffer->cursorByteOffset, 
            buffer->size - buffer->cursorByteOffset + 1
        );
        memcpy(
            &buffer->input[buffer->cursorByteOffset], 
            utf8Symbol, 
            byteSize
        );
        buffer->cursorByteOffset += byteSize;
        buffer->size += byteSize;
    } 
}

void DeleteCharacter(TextBuffer *buffer) {
    if (buffer->cursorByteOffset <= 0) return;
    int bytesToDelete = 1;

    // check if leading bits are continuation bits or not
    while((buffer->cursorByteOffset - bytesToDelete > 0) && ((unsigned char) buffer->input[buffer->cursorByteOffset - bytesToDelete] & 0xC0) == 0x80) {
        bytesToDelete++;
    }

    memmove(
        buffer->input + (buffer->cursorByteOffset - bytesToDelete),
        buffer->input + buffer->cursorByteOffset, 
        buffer->size - buffer->cursorByteOffset + 1
    );

    buffer->cursorByteOffset -= bytesToDelete;
    buffer->size -= bytesToDelete;
}

void DrawCursor(Editor *editor, Vector2 position) {
    Rectangle cursor = {
        .x = position.x, 
        .y = position.y, 
        .width = 1, 
        .height = FONT_SIZE
    };

    if (editor->frameCounter/20 % 2 == 0) {
        DrawRectangleRec(cursor, BLACK);
    }
    
}

void RenderText(Editor *editor) {
    char *ptr = editor->textBuffer->input;
    bool isCursorDrawn = false;
    bool shouldRender = true;

    Vector2 drawPos = {.x = 5, .y = 8};

    BeginScissorMode((int) editor->textBox.x, (int) editor->textBox.y, (int) editor->textBox.width, (int) editor->textBox.height);

    while (shouldRender) {
        int nextCodePoint = GetCodepointNext(ptr, &editor->codepointSize);

        if (nextCodePoint == '\0') {
            shouldRender = false; 
        }

        int charWidth = 0;
        bool isNewLine = (nextCodePoint == '\n');

        if (nextCodePoint != '\0') {
            int index = GetGlyphIndex(editor->textBuffer->font, nextCodePoint);
            charWidth = editor->textBuffer->font.glyphs[index].advanceX;
        }

        if (isNewLine) {
            drawPos.x = 5;
            drawPos.y += FONT_SIZE;
        } else if (nextCodePoint != '\0' && (drawPos.x + charWidth > editor->textBox.width)) {
            drawPos.x = 5;
            drawPos.y += FONT_SIZE;
        }

        Vector2 screenPos = {
            .x = drawPos.x + editor->textBox.x,
            .y = drawPos.y + editor->textBox.y - editor->scrollOffset,
        };

        if (ptr == editor->textBuffer->input + editor->textBuffer->cursorByteOffset) {
            DrawCursor(editor, screenPos);
            editor->cursorPosition = drawPos;
            editor->cursorLayoutY = drawPos.y;
            isCursorDrawn = true;
        }

        if (nextCodePoint == '\0') continue;

        if (!isNewLine) {
            DrawTextCodepoint(editor->textBuffer->font, nextCodePoint, screenPos, FONT_SIZE, BLACK);
            drawPos.x += charWidth;
        } 
        ptr += editor->codepointSize;
    }

    if (!isCursorDrawn) {
        Vector2 screenPos = {
            .x = drawPos.x + editor->textBox.x,
            .y = drawPos.y + editor->textBox.y - editor->scrollOffset,
        };
        editor->cursorPosition = drawPos;
        editor->cursorLayoutY = drawPos.y;
        DrawCursor(editor, screenPos);
    }

    editor->totalContentHeight = drawPos.y + FONT_SIZE;

    EndScissorMode();
}

void HandleScroll(Editor *editor) {
    float maxScroll = editor->totalContentHeight - editor->textBox.height;
    if (maxScroll < 0) maxScroll = 0;
    if (editor->scrollOffset > maxScroll) editor->scrollOffset = maxScroll;

    if (editor->scrollOffset < 0) {
        editor->scrollOffset = 0;
    }
    if (editor->isSearchingCursor) {
        if ( editor->cursorLayoutY + FONT_SIZE > (editor->scrollOffset + editor->textBox.height)) {
            editor->scrollOffset = (editor->cursorLayoutY + FONT_SIZE) - editor->textBox.height;
        } else if (editor->cursorLayoutY < editor->scrollOffset) {
            editor->scrollOffset = editor->cursorLayoutY;
        }
    }
}

void HandleArrowKeys(Editor *editor) {
    editor->isSearchingCursor = true;
    if (IsKeyPressedRepeat(KEY_LEFT) || IsKeyPressed(KEY_LEFT)) {
        if (editor->textBuffer->cursorByteOffset<= 0) {
            editor->textBuffer->cursorByteOffset = 0;
        } else {
            editor->textBuffer->cursorByteOffset--;
            unsigned charAtCursor = (unsigned char)  editor->textBuffer->input[editor->textBuffer->cursorByteOffset];
            //if (charAtCursor == '\n') editor->textBuffer->cursorByteOffset++;
            while ( (charAtCursor & 0xC0) == 0x80 ) {
                editor->textBuffer->cursorByteOffset--;
                charAtCursor = (unsigned char) editor->textBuffer->input[editor->textBuffer->cursorByteOffset];
            }
        }
    }

    if ((IsKeyPressedRepeat(KEY_RIGHT) || IsKeyPressed(KEY_RIGHT)) && editor->textBuffer->cursorByteOffset < editor->textBuffer->size) {
        int byteSize = 0;
        GetCodepointNext(&editor->textBuffer->input[editor->textBuffer->cursorByteOffset], &byteSize);
        editor->textBuffer->cursorByteOffset += byteSize;
    }

    //Disabled temporarily
    if (IsKeyPressed(KEY_UP) && false) {
        int lineCount;
        char *input = editor->textBuffer->input;
        const char **splitText = TextSplit(input, '\n', &lineCount);
        size_t inputStrSize = string_len_utf8(editor->textBuffer->input);
        size_t prevLineLen = 0;
        if (lineCount > 1) {
            prevLineLen = strlen(splitText[lineCount-2]);
        }
        size_t lastLineLen = strlen(splitText[lineCount-1]);

        printf("cursorByteOffset before: %d\n", editor->textBuffer->cursorByteOffset);

        int newOffset = 0;
        if (prevLineLen > lastLineLen) {
            newOffset = editor->textBuffer->cursorByteOffset - lastLineLen - (prevLineLen - lastLineLen);
        } else {
            newOffset = editor->textBuffer->cursorByteOffset - lastLineLen;
        }

        if (newOffset < 0) {
            editor->textBuffer->cursorByteOffset = 0;
        } else {
            editor->textBuffer->cursorByteOffset = newOffset;
        }

        printf("textBuffer size: %d\n", editor->textBuffer->size);
        printf("textBuffer string size: %lu\n", inputStrSize);
        printf("prevLineLen: %lu\n", prevLineLen);
        printf("lastLineLen: %lu\n", lastLineLen);
        printf("cursorByteOffset after: %d\n", editor->textBuffer->cursorByteOffset);
    }

    if (IsKeyPressed(KEY_DOWN)) {
    }
}

size_t string_len_utf8(const char *str) {
    size_t count = 0;
    while (*str) {
        if ((*str & 0xC0) != 0x80 ) {
            count++;
        }
        str++;
    }
    return count;
}
