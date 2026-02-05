#include <assert.h>
#include <raylib.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib/tinyfiledialogs.h"
#include "buffer.h"

#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "renderer/clay_renderer_raylib.h"

#define DARK_GRAY CLITERAL(Color) {48, 48, 48, 255}
#define DARK_GRAY_CLAY  {48, 48, 48, 255}
#define LIGHT_GRAY_CLAY {200, 200, 200, 255}
#define GUTTER_COLOR CLITERAL(Color) {35, 35, 35, 255}
#define GUTTER_COLOR_CLAY  {35, 35, 35, 255}

#define COLOR_BLUE {0, 0, 255, 255}
#define COLOR_ORANGE {255, 255, 0, 255}

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
    const char* title;
    const int width;
    const int height;
    char currentFilePath[1024];
    int codepointSize;
    int frameCounter;
    float scrollOffset;
    float cursorLayoutY;
    float totalContentHeight;
    bool isSearchingCursor;
    bool mouseOnText;
    Font font;
    Vector2 cursorPosition;
    TextBuffer *textBuffer;
    Rectangle gutter;
    Rectangle textBox;
} Editor;

void RenderText(Editor *editor);
void UpdateEditor(Editor* editor, Arena *arena);
void HandleScroll(Editor *editor);
void HandleKeyboardInput(Editor *editor, Arena *arena);
void HandleMouseEvents(Editor *editor, Arena *arena);
int GetIndexFromMouse(Editor *editor, int mouseX, int mouseY);
int min(int x, int y);
int max(int x, int y);
void UpdateTextLayout(Editor *editor, Arena *arena);
void HandleFileDrop(Editor *editor, Arena *arena);
void ClearEditor(Editor *editor);
void LoadFileInEditor(const char *fileName, Editor *editor, Arena *arena);
void HandleFileOpen(Editor *editor, Arena *arena);

Font gutterFont;

bool MenuElementComponent(Clay_String text, Clay_String id, int *cursor) {
    bool clicked = false;
    CLAY(CLAY_SID(id), {.layout = {.sizing = {.width = CLAY_SIZING_FIXED(150), .height = CLAY_SIZING_GROW(0)},
         .padding = CLAY_PADDING_ALL(10)},
         .backgroundColor = GUTTER_COLOR_CLAY}) {
        bool hovered = Clay_Hovered();
        if (hovered) {
            *cursor = MOUSE_CURSOR_POINTING_HAND;
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                clicked = true;
            }
        }
        CLAY_TEXT(text, CLAY_TEXT_CONFIG({.fontSize = 32, .textColor = LIGHT_GRAY_CLAY, .wrapMode = true, 
            .textAlignment = CLAY_TEXT_ALIGN_CENTER}));
    }

    return clicked;
}

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
    textBuffer.rowCapacity = textBuffer.capacity;
    textBuffer.rowStarts = (int *) ArenaAlloc(&textInputArena, textBuffer.rowCapacity * sizeof(int));
    textBuffer.rowCount = 0;

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
            .x = 0, .y = 0, .width = 0, .height = 0
        },
        .textBox = {
            .x = 0, .y = 0, .width = 0, .height = 0
        },
    };
    editor.currentFilePath[0] = '\0';

    int codepoints[512];
    int codepintCount = 0;

    //add ascii unicode character range to codepoints
    for (int i = 32; i < 127; i++) codepoints[codepintCount++] = i;

    //add georgian unicode character range to codepoints
    for (int i = 0x10A0; i < 0x10FF; i++) codepoints[codepintCount++] = i;


    SetTargetFPS(60);
    Clay_Raylib_Initialize(editor.width, editor.height, editor.title, FLAG_WINDOW_RESIZABLE);
    SetExitKey(0);

    uint64_t clayRequiredMemory = Clay_MinMemorySize();
    Clay_Arena clayMemory = (Clay_Arena) {
        .memory = malloc(clayRequiredMemory),
        .capacity = clayRequiredMemory,
    };

    Clay_Initialize(clayMemory, (Clay_Dimensions) {.width = GetScreenWidth(), .height = GetScreenHeight()}, (Clay_ErrorHandler) {});

    editor.font = LoadFontEx(activeFontName, FONT_SIZE, codepoints, codepintCount);
    SetTextureFilter(editor.font.texture, TEXTURE_FILTER_BILINEAR);
    gutterFont = LoadFont(FONT_MONO);
    SetTextureFilter(gutterFont.texture, TEXTURE_FILTER_BILINEAR);

    Font fonts[2] = {editor.font, gutterFont};

    bool fileDropped = false;
    //Clay_SetDebugModeEnabled(true);
    while(!WindowShouldClose()) {
        int cursor = MOUSE_CURSOR_DEFAULT;

        Clay_Vector2 mousePosition = {.x = GetMouseX(), .y = GetMouseY()}; 
        Clay_SetPointerState(mousePosition, true);

        Clay_BeginLayout();

        Clay_SetLayoutDimensions((Clay_Dimensions) {.width = GetScreenWidth(), .height = GetScreenHeight()});


        CLAY(CLAY_ID("MainContainer"), { .layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}, 
            .backgroundColor = DARK_GRAY_CLAY}) {

            CLAY(CLAY_ID("TopMenu"), {.layout = {.layoutDirection = CLAY_LEFT_TO_RIGHT, .sizing = {.width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_FIXED(50)}}, .backgroundColor = DARK_GRAY_CLAY}) {

                if (MenuElementComponent(CLAY_STRING("Open File"), CLAY_STRING("FileOpen"), &cursor)) {
                    HandleFileOpen(&editor, &textInputArena);
                };
                if (MenuElementComponent(CLAY_STRING("Save File"), CLAY_STRING("FileSave"), &cursor)) {
                    //TODO Implement later
                    //HandleFileSave(&editor, &textInputArena);
                }
            }

            CLAY(CLAY_ID("EditorArea"), {.layout = {.layoutDirection = CLAY_LEFT_TO_RIGHT, .sizing = {.width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_GROW(0)}}}) {
                CLAY(CLAY_ID("Gutter"), {.layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = {.width = CLAY_SIZING_FIXED(GUTTER_WIDTH), 
                    .height = CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16), .childGap = 16}, .backgroundColor = GUTTER_COLOR_CLAY}){}

                CLAY(CLAY_ID("TextBox"), {.layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}, 
                     .backgroundColor = LIGHT_GRAY_CLAY}){}
            }

        };

        Clay_RenderCommandArray renderCommands = Clay_EndLayout();

        Clay_ElementData textBoxData =  Clay_GetElementData(Clay_GetElementId(CLAY_STRING("TextBox")));
        Clay_ElementData gutterData =  Clay_GetElementData(Clay_GetElementId(CLAY_STRING("Gutter")));
        editor.textBox.x = textBoxData.boundingBox.x;
        editor.textBox.y = textBoxData.boundingBox.y;
        editor.textBox.width = textBoxData.boundingBox.width;
        editor.textBox.height = textBoxData.boundingBox.height;

        editor.gutter.x = gutterData.boundingBox.x;
        editor.gutter.y = gutterData.boundingBox.y;
        editor.gutter.width = gutterData.boundingBox.width;
        editor.gutter.height = gutterData.boundingBox.height;

        UpdateEditor(&editor, &textInputArena);

        if (cursor == MOUSE_CURSOR_DEFAULT && editor.mouseOnText) {
            cursor = MOUSE_CURSOR_IBEAM;
        }

        SetMouseCursor(cursor);

        BeginDrawing();
        Clay_Raylib_Render(renderCommands, fonts);

        UpdateTextLayout(&editor, &textInputArena);
        RenderText(&editor);

        HandleFileDrop(&editor, &textInputArena);

        if (!editor.textBuffer->isSaved) {
            SetWindowTitle("Ravi Editor *");
        } else {
            SetWindowTitle("Ravi Editor");
        }

        EndDrawing();
    }

    CloseWindow();

    return 0;
}

void UpdateEditor(Editor* editor, Arena *arena) {
    HandleScroll(editor);

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_F)) {
        MaximizeWindow();
    }

    HandleKeyboardInput(editor, arena);
    HandleMouseEvents(editor, arena);

    if (editor->mouseOnText) {
        editor->frameCounter++;
        //SetMouseCursor(MOUSE_CURSOR_IBEAM);
        int key = GetCharPressed();

        while (key > 0) {
            InsertCharacter(editor->textBuffer, arena, key);
            key = GetCharPressed();
        }

    } else {
        editor->frameCounter = 0;
        //SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }
}

void DrawCursor(Editor *editor, Vector2 position) {
    Rectangle cursor = {
        .x = position.x, 
        .y = position.y, 
        .width = 3, 
        .height = FONT_SIZE
    };

    if (editor->frameCounter/20 % 2 == 0) {
        DrawRectangleRec(cursor, BLACK);
    }
}

void RenderText(Editor *editor) {
    TextBuffer *buffer = editor->textBuffer;

    Vector2 cursorScreenPosition = (Vector2) {
        .x = editor->cursorPosition.x + editor->textBox.x,
        .y = editor->cursorPosition.y + editor->textBox.y - editor->scrollOffset
    };

    DrawCursor(editor, cursorScreenPosition);

    BeginScissorMode((int)editor->textBox.x, (int)editor->textBox.y, (int)editor->textBox.width, (int)editor->textBox.height);
    for (int i = 0; i < editor->textBuffer->rowCount; i++) {
        float rowY = editor->textBox.y + TEXT_OFFSET_Y + (i * FONT_SIZE) - editor->scrollOffset;
        if (rowY < editor->textBox.y - FONT_SIZE || rowY > editor->textBox.y + editor->textBox.height) {
            continue;
        }

        int start = buffer->rowStarts[i];
        int end = (i < buffer->rowCount - 1) ? buffer->rowStarts[i+1] : buffer->size;

        float currentX = TEXT_OFFSET_X;

        int highlightStart = min(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
        int highlightEnd = max(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
        while (start < end) {
            char *ptr = buffer->input + start;
            int nextCodePoint = GetCodepointNext(ptr, &editor->codepointSize);

            if (nextCodePoint == '\n') {
                start += editor->codepointSize;
                continue;
            }

            Vector2 charPosition = (Vector2) {
                .x = currentX + editor->textBox.x,
                .y = rowY
            };

            int index = GetGlyphIndex(editor->font, nextCodePoint);
            int charWidth = editor->font.glyphs[index].advanceX;

            if (start >= highlightStart && start < highlightEnd) {
                DrawRectangle(charPosition.x, charPosition.y, charWidth, FONT_SIZE, SKYBLUE);
            }

            if (nextCodePoint == '\t') {
                int index = GetGlyphIndex(editor->font, ' ');
                int spaceWidth = editor->font.glyphs[index].advanceX;
                int advance = (4 - ((int) (currentX / spaceWidth) % 4)) * spaceWidth;
                currentX += advance;
            } else {
                DrawTextCodepoint(editor->font, nextCodePoint, charPosition, FONT_SIZE, BLACK);
                currentX += charWidth;
            }

            start += editor->codepointSize;
        }
    }
    EndScissorMode();

    Vector2 gutterPos = {
        .x = editor->gutter.x + ((float) GUTTER_WIDTH / 2),
    };
    
    BeginScissorMode(editor->gutter.x, editor->gutter.y, editor->gutter.width, editor->gutter.height);
    int lineNumber = 1;
    for (int i = 0; i < buffer->rowCount; i++) {
        if (i > 0 && buffer->input[buffer->rowStarts[i] - 1] == '\n') {
           lineNumber ++; 
        }

        float y = editor->gutter.y + TEXT_OFFSET_Y + (i * FONT_SIZE) - editor->scrollOffset;
        if (y < editor->gutter.y - FONT_SIZE || y > editor->gutter.y + editor->gutter.height) {
            continue;
        }

        if (i == 0 || buffer->input[buffer->rowStarts[i] - 1] == '\n') {
            gutterPos.y = y;
            DrawTextEx(gutterFont, TextFormat("%d", lineNumber), gutterPos, FONT_SIZE, 1, LIGHTGRAY);
        }

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
        editor->isSearchingCursor = true;
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

    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
        editor->isSearchingCursor = true;
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

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
        editor->isSearchingCursor = true;
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

    if (IsKeyPressed(KEY_TAB) || IsKeyPressedRepeat(KEY_TAB)) {
        editor->isSearchingCursor = true;
        InsertCharacter(editor->textBuffer, arena, '\t');
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

    if (IsKeyPressed(KEY_DELETE) || IsKeyPressedRepeat(KEY_DELETE)) {
        editor->isSearchingCursor = true;
        if (editor->textBuffer->cursorByteOffset != editor->textBuffer->selectionAnchor) {
            int start = min(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
            int end = max(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);

            DeleteRange(editor->textBuffer, start, end);
        } else {
            if (editor->textBuffer->cursorByteOffset >= editor->textBuffer->size) return;

            int byteSize = 0;
            GetCodepoint(&editor->textBuffer->input[editor->textBuffer->cursorByteOffset], &byteSize);
            DeleteRange(editor->textBuffer, editor->textBuffer->cursorByteOffset, editor->textBuffer->cursorByteOffset + byteSize);
        }
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

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_A)) {
        editor->textBuffer->cursorByteOffset = 0;
        editor->textBuffer->selectionAnchor = editor->textBuffer->size;
    }
    
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) {
        char *targetPath = NULL;
        if (editor->currentFilePath[0] == '\0') {
            const char *defaultName = "new_file.txt";
            const char *selectedPath = tinyfd_saveFileDialog("Choose file to save", defaultName, 0, NULL, NULL);
            if (selectedPath != NULL) {
                targetPath = strncpy(editor->currentFilePath, selectedPath, 1024);
            }
        } else {
            targetPath = editor->currentFilePath;
        }

        if (targetPath!=NULL) {
            bool saved = SaveFileData(targetPath, editor->textBuffer->input, editor->textBuffer->size);
            if (!saved) {
                printf("file not saved!\n");
            }

            if (editor->textBuffer->size > 0) {
                editor->textBuffer->isSaved = true;
            }
        }
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) {
        HandleFileOpen(editor, arena);
    }
}

void HandleMouseEvents(Editor *editor, Arena *arena) {
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

        //if (Clay_PointerOver(Clay_GetElementId(CLAY_STRING("FileOpen"))) ) {
        //    HandleFileOpen(editor, arena);
        //}
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        int index = GetIndexFromMouse(editor, GetMouseX(), GetMouseY());
        editor->textBuffer->cursorByteOffset = index;
    }
}

int GetIndexFromMouse(Editor *editor, int mouseX, int mouseY) {
    TextBuffer *buffer = editor->textBuffer;
    int offset = buffer->cursorByteOffset;

    int targetRow = (mouseY - (editor->textBox.y + TEXT_OFFSET_Y) + (int)editor->scrollOffset) / FONT_SIZE;
    targetRow = max(targetRow, 0);

    if (targetRow >= buffer->rowCount) targetRow = buffer->rowCount - 1;

    int start = buffer->rowStarts[targetRow];
    int end   = (targetRow < buffer->rowCount - 1) ? buffer->rowStarts[targetRow + 1] : buffer->size;
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
        int charWidth = 0;

        if (codePoint == '\t') {
            int index = GetGlyphIndex(editor->font, ' ');
            int spaceWidth = editor->font.glyphs[index].advanceX;
            charWidth = (4 - ((int)((currentPixelWidth - (editor->textBox.x)) / spaceWidth) % 4)) * spaceWidth;
        } else {
            int index = GetGlyphIndex(editor->font, codePoint);
            charWidth = editor->font.glyphs[index].advanceX;
        }

        if (mouseX < currentPixelWidth + (charWidth / 2)) {
            return i;
        }

        currentPixelWidth += charWidth;
        i+=byteSize;
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

void UpdateTextLayout(Editor *editor, Arena *arena) {
    editor->textBuffer->rowCount = 0;
    PushRowStarts(editor->textBuffer, arena, 0);

    char *ptr = editor->textBuffer->input;
    bool running = true;

    Vector2 layoutCursor = {.x = TEXT_OFFSET_X, .y = TEXT_OFFSET_Y};

    while (running) {
        int nextCodePoint = GetCodepointNext(ptr, &editor->codepointSize);

        if (nextCodePoint == '\0') {
            running = false; 
        }

        int charWidth = 0;
        bool isNewLine = (nextCodePoint == '\n');
        bool isTab = (nextCodePoint == '\t');

        if (nextCodePoint != '\0') {
            int index = GetGlyphIndex(editor->font, nextCodePoint);
            charWidth = editor->font.glyphs[index].advanceX;
        }

        if (!isNewLine && nextCodePoint != '\0' && (layoutCursor.x + charWidth > editor->textBox.width)) {
            layoutCursor.x = TEXT_OFFSET_X;
            layoutCursor.y += FONT_SIZE;

            int currentIndex = (int) (ptr - editor->textBuffer->input);
            PushRowStarts(editor->textBuffer, arena, currentIndex);
        }

        if (ptr == editor->textBuffer->input + editor->textBuffer->cursorByteOffset) {
            editor->cursorPosition = layoutCursor;
            editor->cursorLayoutY = layoutCursor.y;
        }

        if (nextCodePoint == '\0') break;

        if (isNewLine) {
            layoutCursor.x = TEXT_OFFSET_X;
            layoutCursor.y += FONT_SIZE;

            int nextIndex = (int) (ptr - editor->textBuffer->input) + 1;
            PushRowStarts(editor->textBuffer, arena, nextIndex);
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
    editor->totalContentHeight = layoutCursor.y + FONT_SIZE + PADDING;
}

void HandleFileOpen(Editor *editor, Arena *arena) {
    char *fileName = tinyfd_openFileDialog("Open file", "", 0, NULL, NULL, 0);
    if (fileName != NULL) {
        ClearEditor(editor);
        LoadFileInEditor(fileName, editor, arena);
    }
}

void HandleFileDrop(Editor *editor, Arena *arena) {
    FilePathList filePathList = {0};
    if (IsFileDropped()) {
        filePathList = LoadDroppedFiles();
        
        if (filePathList.count > 0) {
            const char *fileName = filePathList.paths[0];
            ClearEditor(editor);
            LoadFileInEditor(fileName, editor, arena);
        }

        UnloadDroppedFiles(filePathList);
    }
}

void LoadFileInEditor(const char *fileName, Editor *editor, Arena *arena) {
    int size = 0;
    unsigned char *fileData = LoadFileData(fileName, &size);

    if (fileData != NULL) {
        InsertBytes(editor->textBuffer, arena, (char *) fileData, size);
        strncpy(editor->currentFilePath, fileName, 1024);
        UnloadFileData(fileData);
    }
}

void ClearEditor(Editor *editor) {
    editor->textBuffer->input[0] = '\0';
    editor->currentFilePath[0] = '\0';

    editor->textBuffer->size = 0;
    editor->textBuffer->rowCount = 0;
    editor->textBuffer->cursorByteOffset = 0;
    editor->textBuffer->selectionAnchor = 0;
    editor->scrollOffset = 0;

    editor->totalContentHeight = 0;

    editor->textBuffer->isSaved = true;
}
