#include <assert.h>
#include <raylib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DARK_GRAY CLITERAL(Color) {48, 48, 48, 255}
#define GUTTER_COLOR CLITERAL(Color) {35, 35, 35, 255}
#define MAX_INPUT_CHAR 100000
#define FONT_SIZE 40
#define PADDING 40
#define GUTTER_WIDTH 100
#define TEXT_OFFSET_X 5
#define TEXT_OFFSET_Y 5
#define TEXT_ARENA_SIZE 100 * 1024 * 1024

#define FONT_SANS "resources/fonts/NotoSansGeorgian-Regular.ttf"
#define FONT_MONO "resources/fonts/FiraCode-Medium.ttf"

typedef struct {
    unsigned char *memory;
    size_t capacity;
    size_t used;
} Arena;

typedef struct {
    char* input;
    size_t size;
    size_t capacity;
    int cursorByteOffset;
    int selectionAnchor;
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
    Rectangle gutter;
    Rectangle textBox;
} Editor;

void InsertCharacter(TextBuffer *buffer, Arena *arena, int key);
void DeleteCharacter(TextBuffer *buffer);
void DeleteRange(TextBuffer *buffer, int start, int end);
void RenderText(Editor *editor);
void UpdateEditor(Editor* editor, Arena *arena);
void HandleScroll(Editor *editor);
void HandleKeyboardInput(Editor *editor, Arena *arena);
void HandleMouseEvents(Editor *editor);
size_t string_len_utf8(const char *str);
int GetLineStart(char *buffer, int currentOffset);
int GetLineEnd(char *buffer, int currentOffset, size_t bufferSize);
int GetWordStart(char *buffer, int currentOffset);
int GetWordEnd(char *buffer, int currentOffset, size_t bufferSize);
int GetPreviousCharSize(char *buffer, int currentOffset);
size_t safe_strlen(const char *s, size_t max_len);
void InsertBytes(TextBuffer *buffer, Arena *arena, const char *data, size_t size);
int GetIndexFromMouse(Editor *editor, int mouseX, int mouseY);
int min(int x, int y);
int max(int x, int y);
unsigned char* ArenaAlloc(Arena *arena, size_t size);

Font gutterFont;

int main(void) {
    Arena textInputArena = (Arena) {
        .memory = malloc(TEXT_ARENA_SIZE),
        .used = 0,
        .capacity = TEXT_ARENA_SIZE
    };
    
    const char * activeFontName = FONT_SANS;

    TextBuffer textBuffer = {
        .capacity = MAX_INPUT_CHAR,
        .cursorByteOffset = 0,
        .size = 0,
    };

    textBuffer.input = (char *) ArenaAlloc(&textInputArena, textBuffer.capacity);
    textBuffer.input[0] = '\0';

    Editor editor = {
        .title = "Ravi Editor",
        .width = 1920,
        .height = 1200,
        .codepointSize = 0,
        .frameCounter = 0,
        .scrollOffset = 0.0,
        .cursorLayoutY = 0.0,
        .isSearchingCursor = false,
        .mouseOnText = false,
        .textBuffer = &textBuffer,
        .gutter = {
            .x = PADDING,
            .y = PADDING,
        },
        .textBox = {
            .x = PADDING + GUTTER_WIDTH,
            .y = PADDING,
        },
    };
    editor.textBox.width = editor.width - (PADDING + GUTTER_WIDTH) * 2;
    editor.textBox.height = editor.height - PADDING * 2;
    editor.gutter.width = GUTTER_WIDTH;
    editor.gutter.height= editor.textBox.height; 

    int codepoints[512];
    int codepintCount = 0;

    //add ascii unicode character range to codepoints
    for (int i = 32; i < 127; i++) codepoints[codepintCount++] = i;

    //add georgian unicode character range to codepoints
    for (int i = 0x10A0; i < 0x10FF; i++) codepoints[codepintCount++] = i;


    SetTargetFPS(60);
    InitWindow(editor.width, editor.height, editor.title);
    SetExitKey(0);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    textBuffer.font = LoadFontEx(activeFontName, FONT_SIZE, codepoints, codepintCount);
    SetTextureFilter(textBuffer.font.texture, TEXTURE_FILTER_BILINEAR);
    gutterFont = LoadFont(FONT_MONO);

    while(!WindowShouldClose()) {
        UpdateEditor(&editor, &textInputArena);

        BeginDrawing();

        ClearBackground(DARK_GRAY);
        DrawRectangleRec(editor.textBox, LIGHTGRAY);
        DrawRectangleRec(editor.gutter, GUTTER_COLOR);
        RenderText(&editor);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}

void UpdateEditor(Editor* editor, Arena *arena) {
    HandleScroll(editor);

    if (IsWindowResized()) {
        editor->textBox.width = GetScreenWidth() - PADDING * 2;
        editor->textBox.height = GetScreenHeight() - PADDING * 2;
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_F)) {
        MaximizeWindow();
    }

    HandleKeyboardInput(editor, arena);
    HandleMouseEvents(editor);

    if (editor->mouseOnText) {
        editor->frameCounter++;
        SetMouseCursor(MOUSE_CURSOR_IBEAM);
        int key = GetCharPressed();

        while (key > 0) {
            InsertCharacter(editor->textBuffer, arena, key);
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            editor->isSearchingCursor = true;
            if (editor->textBuffer->cursorByteOffset != editor->textBuffer->selectionAnchor) {
                int start = min(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
                int end = max(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);

                DeleteRange(editor->textBuffer, start, end);

            } else {
                DeleteCharacter(editor->textBuffer);
            }
        }

    } else {
        editor->frameCounter = 0;
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }
}

void InsertCharacter(TextBuffer *buffer, Arena *arena, int key) {
    int byteSize = 0;
    const char *utf8Symbol = CodepointToUTF8(key, &byteSize);
    InsertBytes(buffer, arena, utf8Symbol, byteSize);
}

void DeleteCharacter(TextBuffer *buffer) {
    if (buffer->cursorByteOffset <= 0) return;

    int bytesToDelete = GetPreviousCharSize(buffer->input, buffer->cursorByteOffset);

    DeleteRange(buffer, buffer->cursorByteOffset - bytesToDelete, buffer->cursorByteOffset);
}

void DeleteRange(TextBuffer *buffer, int start, int end) {
    if (start >= end)return;

    int count = end - start;

    memmove(
        buffer->input + start,
        buffer->input + end, 
        buffer->size - end + 1
    );

    buffer->cursorByteOffset = start;
    buffer->selectionAnchor = buffer->cursorByteOffset; 
    buffer->size -= count;
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
    static int linePositions[MAX_INPUT_CHAR] = {};
    int lineNumber = 0;

    Vector2 drawPos = {.x = TEXT_OFFSET_X, .y = TEXT_OFFSET_Y};

    linePositions[0] = TEXT_OFFSET_Y + editor->textBox.y - editor->scrollOffset;


    int start = min(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
    int end = max(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
    int currentIndex = 0;

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

        if (!isNewLine && nextCodePoint != '\0' && (drawPos.x + charWidth > editor->textBox.width)) {
            drawPos.x = TEXT_OFFSET_X;
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

        if (nextCodePoint == '\0') break;

        if (currentIndex >= start && currentIndex < end) {
            DrawRectangle(screenPos.x, screenPos.y, charWidth, FONT_SIZE, SKYBLUE);
        }

        currentIndex += editor->codepointSize;

        if (isNewLine) {
            drawPos.x = TEXT_OFFSET_X;
            drawPos.y += FONT_SIZE;
            lineNumber++;
            linePositions[lineNumber] = drawPos.y + editor->textBox.y - editor->scrollOffset;
        } else {
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

    Vector2 gutterPos = {
        .x = editor->gutter.x + ((float) GUTTER_WIDTH / 2),
    };

    BeginScissorMode(editor->gutter.x, editor->gutter.y, editor->gutter.width, editor->gutter.height);
    for (int i = 0; i <= lineNumber; i++) {
        float y = linePositions[i];

        if (y < editor->gutter.y - FONT_SIZE || y > editor->gutter.y + editor->gutter.height) {
            continue;
        }

        gutterPos.y = y;

        DrawTextEx(gutterFont, TextFormat("%d", i+1), gutterPos, FONT_SIZE, 1, LIGHTGRAY);
    }
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

void HandleKeyboardInput(Editor *editor, Arena *arena) {
    editor->isSearchingCursor = true;

    if (IsKeyPressed(KEY_ESCAPE)) {
        editor->textBuffer->selectionAnchor = editor->textBuffer->cursorByteOffset;
    }

    if (IsKeyPressedRepeat(KEY_LEFT) || IsKeyPressed(KEY_LEFT)) {
        if (editor->textBuffer->cursorByteOffset<= 0) {
            editor->textBuffer->cursorByteOffset = 0;
            if (!IsKeyDown(KEY_LEFT_SHIFT)) {
                editor->textBuffer->selectionAnchor = 0;
            }
        } else {
            int cursorBeforeCtrl = editor->textBuffer->cursorByteOffset;

            int charSize = GetPreviousCharSize(editor->textBuffer->input, editor->textBuffer->cursorByteOffset);
            editor->textBuffer->cursorByteOffset -= charSize;

            int wordStart = GetWordStart(editor->textBuffer->input, editor->textBuffer->cursorByteOffset);
            int wordEnd = GetWordEnd(editor->textBuffer->input, editor->textBuffer->cursorByteOffset, editor->textBuffer->size);
            if (IsKeyDown(KEY_LEFT_CONTROL)) {
                editor->textBuffer->cursorByteOffset = wordStart;
            }
            if (!IsKeyDown(KEY_LEFT_SHIFT)) {
                editor->textBuffer->selectionAnchor = editor->textBuffer->cursorByteOffset;
            }
        }
    }

    if ((IsKeyPressedRepeat(KEY_RIGHT) || IsKeyPressed(KEY_RIGHT)) && editor->textBuffer->cursorByteOffset < editor->textBuffer->size) {
        int byteSize = 0;
        GetCodepoint(&editor->textBuffer->input[editor->textBuffer->cursorByteOffset], &byteSize);

        int cursorBeforeCtrl = editor->textBuffer->cursorByteOffset;
        editor->textBuffer->cursorByteOffset += byteSize;

        int wordStart = GetWordStart(editor->textBuffer->input, editor->textBuffer->cursorByteOffset);
        int wordEnd = GetWordEnd(editor->textBuffer->input, editor->textBuffer->cursorByteOffset, editor->textBuffer->size);
        if (IsKeyDown(KEY_LEFT_CONTROL)) {
            editor->textBuffer->cursorByteOffset = wordEnd;
        }

        if (!IsKeyDown(KEY_LEFT_SHIFT)) {
            editor->textBuffer->selectionAnchor = editor->textBuffer->cursorByteOffset;
        }
    }

    if (IsKeyPressed(KEY_UP)) {
        int startOfCurrentLine = GetLineStart(editor->textBuffer->input, editor->textBuffer->cursorByteOffset);
        int currentColumn = editor->textBuffer->cursorByteOffset - startOfCurrentLine;

        int prevLineOffset = editor->textBuffer->cursorByteOffset - currentColumn - 1;

        if (prevLineOffset < 0) return;
        int startOfPrevLine = GetLineStart(editor->textBuffer->input, prevLineOffset);

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

    if (IsKeyPressed(KEY_DOWN)) {
        int startOfCurrentLine = GetLineStart(editor->textBuffer->input, editor->textBuffer->cursorByteOffset);
        int currentColumn = editor->textBuffer->cursorByteOffset - startOfCurrentLine;

        int endOfCurrentLine = GetLineEnd(editor->textBuffer->input, editor->textBuffer->cursorByteOffset, editor->textBuffer->size);

        if (endOfCurrentLine >= editor->textBuffer->size) return;

        int startOfNextLine =  endOfCurrentLine + 1;

        int nextLineEnd = GetLineEnd(editor->textBuffer->input, startOfNextLine, editor->textBuffer->size);
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
        InsertCharacter(editor->textBuffer, arena, '\n');
    }

    // copy
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_C)) {
        int start = min(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
        int end = max(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);

        if (start != end) {
            size_t size = end - start;
            size_t usedBeforeCopy = arena->used;

            char *highlightedText = (char *)ArenaAlloc(arena, size + 1);
            memcpy(highlightedText, &editor->textBuffer->input[start], size);

            highlightedText[size] = '\0';

            SetClipboardText(highlightedText);

            arena->used = usedBeforeCopy;
        }
    }

    // paste
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V)) {
        const char *clipboardText = GetClipboardText();

        if (clipboardText != NULL) {
            size_t pastedSize = strlen(clipboardText);
            InsertBytes(editor->textBuffer, arena, clipboardText, pastedSize);
        }
    }
}

void HandleMouseEvents(Editor *editor) {
    if (CheckCollisionPointRec(GetMousePosition(), editor->textBox)) {
        editor->mouseOnText = true;
    } else {
        editor->mouseOnText = false;
    }

    if (GetMouseWheelMove() != 0) {
        editor->scrollOffset -= GetMouseWheelMove() * FONT_SIZE;
        editor->isSearchingCursor = false;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        int index = GetIndexFromMouse(editor, GetMouseX(), GetMouseY());
        editor->textBuffer->cursorByteOffset = index;
        editor->textBuffer->selectionAnchor = index;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        int index = GetIndexFromMouse(editor, GetMouseX(), GetMouseY());
        editor->textBuffer->cursorByteOffset = index;
    }
}

int GetLineStart(char *buffer, int currentOffset) {
    if (currentOffset <= 0) return 0;

    int scanIndex = currentOffset;

    while(scanIndex > 0) {
        scanIndex --;
        if (buffer[scanIndex] == '\n') {
            return scanIndex + 1;
        }
    }

    return 0;
}

int GetLineEnd(char *buffer, int currentOffset, size_t bufferSize) {
    if (currentOffset < 0) return 0;
    if (bufferSize < currentOffset) return 0;

    int scanIndex = currentOffset;

    while(scanIndex < bufferSize) {
        if (buffer[scanIndex] == '\n') {
            return scanIndex;
        }
        scanIndex ++;
    }

    return bufferSize;
}

int GetWordStart(char *buffer, int currentOffset) {
    if (currentOffset <= 0) return 0;

    int scanIndex = currentOffset;

    while(scanIndex > 0 && (buffer[scanIndex - 1] == ' ' || buffer[scanIndex - 1] == '\n')) {
        scanIndex --;
    }

    while(scanIndex > 0 && (buffer[scanIndex - 1] != ' ' && buffer[scanIndex - 1] != '\n')) {
        scanIndex --;
    }

    return scanIndex;
}

int GetWordEnd(char *buffer, int currentOffset, size_t bufferSize) {
    if (currentOffset < 0) return 0;
    if (bufferSize < currentOffset) return 0;

    int scanIndex = currentOffset;

    while(scanIndex < bufferSize && (buffer[scanIndex] != ' ' && buffer[scanIndex] != '\n')) {
        scanIndex ++;
    }

    while(scanIndex < bufferSize && (buffer[scanIndex] == ' ' || buffer[scanIndex] == '\n')) {
        scanIndex ++;
    }

    return scanIndex;
}

int GetPreviousCharSize(char *buffer, int currentOffset) {
    if (currentOffset <= 0) return 0;

    int charSize = 1;
    currentOffset--;

    while (currentOffset > 0 && (buffer[currentOffset] & 0xC0) == 0x80 ) {
        charSize ++;
        currentOffset--;
    }

    return charSize;
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

size_t safe_strlen(const char *s, size_t max_len) {
    size_t length = 0;
    if (s == NULL) { // Check for null pointer
        return 0;
    }
    while (length < max_len && s[length] != '\0') { // Check bounds and terminator
        length++;
    }
    return length;
}

void InsertBytes(TextBuffer *buffer, Arena *arena, const char *data, size_t size) {
    if (buffer->size + size > buffer->capacity) {
        size_t newCapacity = (buffer->capacity + size) * 2;
        char *newPtr = (char *)ArenaAlloc(arena, newCapacity);

        memcpy(newPtr, buffer->input, buffer->size + 1);

        buffer->input = newPtr;
        buffer->capacity = newCapacity;
    }

    memmove(
        buffer->input + buffer->cursorByteOffset + size,
        buffer->input + buffer->cursorByteOffset, 
        buffer->size - buffer->cursorByteOffset + 1
    );
    memcpy(
        &buffer->input[buffer->cursorByteOffset], 
        data, 
        size
    );
    buffer->cursorByteOffset += size;
    buffer->size += size;
    buffer->selectionAnchor = buffer->cursorByteOffset;
}

int GetIndexFromMouse(Editor *editor, int mouseX, int mouseY) {
    int offset = editor->textBuffer->cursorByteOffset;

    int targetRow = (mouseY - (PADDING + TEXT_OFFSET_Y) + (int)editor->scrollOffset) / FONT_SIZE;
    targetRow = max(targetRow, 0);

    int lineNumber = 0;
    int startOfLineIndex = 0;
    if (targetRow > 0) {
        for (int i = 0; i < editor->textBuffer->size; i++) {
            if (editor->textBuffer->input[i] == '\n') {
                lineNumber++;
                if (lineNumber == targetRow) {
                    startOfLineIndex = i + 1;
                    break;
                }
            }
        }
        if (lineNumber < targetRow) {
            startOfLineIndex = editor->textBuffer->size;
        }
    }

    int currentPixelWidth = PADDING + GUTTER_WIDTH + TEXT_OFFSET_X;
    int i = startOfLineIndex;
    while (i < editor->textBuffer->size) {
        char *input = editor->textBuffer->input;
        if (input[i] == '\n' || input[i] == '\0') {
            editor->textBuffer->cursorByteOffset = i;
            break;
        }

        int byteSize = 0;
        int codePoint = GetCodepoint(&input[i], &byteSize);
        int index = GetGlyphIndex(editor->textBuffer->font, codePoint);
        int charWidth = editor->textBuffer->font.glyphs[index].advanceX;

        currentPixelWidth += charWidth;
        if (mouseX < currentPixelWidth - (charWidth / 2)) {
            editor->textBuffer->cursorByteOffset = i;
            break;
        } else {
            i+=byteSize;
        }
    }

    return i;
}

int min(int x, int y) {
    if (x > y) return y;
    return x;
}

int max(int x, int y) {
    if (x < y) return y;
    return x;
}

unsigned char* ArenaAlloc(Arena *arena, size_t size) {
    assert(arena->used + size <= arena->capacity);

    void *ptr = arena->memory + arena->used;
    arena->used += size;

    return ptr;
}
