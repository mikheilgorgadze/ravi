#include <assert.h>
#include <raylib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/tinyfiledialogs.h"
#include "buffer.h"
#include "history.h"
#include "lexer.h"
#include "arena.h"
#include "../resources/fonts/firacode.h"
#include "../resources/fonts/notosans.h"
#include "../resources/logo.h"

#define CLAY_IMPLEMENTATION
#include "../lib/clay.h"
#include "../renderer/clay_renderer_raylib.h"

#define DARK_GRAY CLITERAL(Color) {48, 48, 48, 255}
#define DARK_GRAY_CLAY  {48, 48, 48, 255}
#define LIGHT_GRAY_CLAY {200, 200, 200, 255}
#define GUTTER_COLOR CLITERAL(Color) {35, 35, 35, 255}
#define GUTTER_COLOR_CLAY  {35, 35, 35, 255}

#define COLOR_BLUE {0, 0, 255, 255}
#define COLOR_ORANGE {255, 255, 0, 255}
#define COLOR_RED {255, 0, 0, 255}

#define MAX_INPUT_CHAR 10000000
#define PADDING 40
#define TEXT_OFFSET_X 5
#define TEXT_OFFSET_Y 5
#define TEXT_ARENA_SIZE 1024 * 1024 * 1024

#define FONT_SANS "resources/fonts/NotoSansGeorgian-Regular.ttf"
#define FONT_MONO "resources/fonts/FiraCode-Regular.ttf"

typedef enum {
    MODE_NORMAL,
    MODE_SEARCH,
    MODE_PROMPT,
} EditorMode;

typedef struct {
    const char* title;
    int width;
    int height;
    char currentFilePath[1024];
    long lastModificationTime;
    int codepointSize;
    int frameCounter;
    int fontSize;
    float gutterWidth;
    float scrollOffset;
    float totalContentHeight;

    text_buffer_t *searchBuffer;
    EditorMode editorMode;

    int codepointsASCII[256];
    int codepointsCountASCII;

    int codepointsGeo[256];
    int codepointsCountGeo;

    syntax_token_list_t syntaxTokens;
    history_buffer_t historyBuffer;

    bool isSearchingCursor;
    bool isScrolling;
    bool mouseOnText;
    bool isUpdateNeeded;
    Font font;
    Font gutterFont;
    Font fallbackFont;
    Vector2 cursorPosition;
    text_buffer_t *textBuffer;
    Rectangle gutter;
    Rectangle textBox;
    Rectangle searchBox;
} Editor;

keyword_t keywords[] = {
    {.name = "int",             .token_type = TOKEN_DATA_TYPE},
    {.name = "float",           .token_type = TOKEN_DATA_TYPE},
    {.name = "long",            .token_type = TOKEN_DATA_TYPE},
    {.name = "char",            .token_type = TOKEN_DATA_TYPE},
    {.name = "void",            .token_type = TOKEN_DATA_TYPE},
    {.name = "bool",            .token_type = TOKEN_DATA_TYPE},
    {.name = "typedef",         .token_type = TOKEN_KEYWORD},
    {.name = "struct",          .token_type = TOKEN_KEYWORD},
    {.name = "const",           .token_type = TOKEN_KEYWORD},
    {.name = "#include",        .token_type = TOKEN_KEYWORD},
    {.name = "#define",         .token_type = TOKEN_KEYWORD},
    {.name = "if",              .token_type = TOKEN_KEYWORD},
	{.name = "else",            .token_type = TOKEN_KEYWORD},
    {.name = "while",           .token_type = TOKEN_KEYWORD},
    {.name = "for",             .token_type = TOKEN_KEYWORD},
    {.name = "static",          .token_type = TOKEN_KEYWORD},
    {.name = "string_literal",  .token_type = TOKEN_STRING_LITERAL},
    {.name = "single_quotes",   .token_type = TOKEN_SINGLE_QUOTE},
    {.name = "comment",         .token_type = TOKEN_COMMENT},
    {0},
};

// maps token types to colors
Color color_theme[10] = {
    RED,
    DARKGREEN,
    GREEN,
    ORANGE,
    PURPLE,
    PURPLE,
    GRAY,
};



void RenderText(Editor *editor);
void RenderSearchBarText(Editor *editor);
void UpdateEditor(Editor* editor, Clay_ElementData elementData);
void HandleScroll(Editor *editor);
void HandleKeyboardInput(Editor *editor);
void HandleNormalMode(Editor *editor);
void HandleSearchMode(Editor *editor);
void HandlePromptMode(Editor *editor);
void HandleMouseEvents(Editor *editor, Clay_ElementData elementData);
int GetIndexFromMouse(Editor *editor, int mouseX, int mouseY);
int min(int x, int y);
int max(int x, int y);
void UpdateTextLayout(Editor *editor);
void HandleFileDrop(Editor *editor);
void ClearEditor(Editor *editor);
void LoadFileInEditor(const char *fileName, Editor *editor);
void HandleFileOpen(Editor *editor);
Font* GetFontForCodepoint(Editor *editor, int codepoint);
bool ButtonComponent(Clay_String text, Clay_String id, int *cursor, float width, Clay_Color color);
void RenderScrollbar(Editor *editor, Clay_ElementData elementData);
bool IsSeparator(int codepoint);
void HandleFileSave(Editor *editor);
void Copy(Editor *editor, int start, int end);
void UpdateCursorPosition(Editor *editor);
void HandleFontLoad(Font *currentFont, const unsigned char *fileData, int dataSize, int fontSize, int *codepoints, int codepointCount);
void RecordDeleteAction(Editor *editor, int startOffset, int length);
void RecordInsertAction(Editor *editor, const char *text, int length);
void RenderGutter(Editor *editor);
void FindNextMatch(Editor *editor, int startOffset);
void Zoom(Editor *editor, float zoomAmt);
void Scroll(Editor *editor, float scrollAmt);

int main(int argc, char *argv[]) {

    arena_t textInputArena = (arena_t) {
        .base_memory = malloc(TEXT_ARENA_SIZE / 2),
        .used = 0,
        .capacity = TEXT_ARENA_SIZE / 2
    };

    arena_t scratchArena = (arena_t) {
        .base_memory = malloc(TEXT_ARENA_SIZE / 2),
        .used = 0,
        .capacity = TEXT_ARENA_SIZE / 2
    };

    text_buffer_t textBuffer = {
        .capacity = MAX_INPUT_CHAR,
        .cursorByteOffset = 0,
        .size = 0,
        .isSaved = true,
    };

    text_buffer_t searchBuffer = {
        .capacity = 256,
        .cursorByteOffset = 0,
        .size = 0
    };

    textBuffer.input = (char *) arena_alloc(&textInputArena, textBuffer.capacity);
    textBuffer.input[0] = '\0';
    textBuffer.rowList.count = 0;

    searchBuffer.input = (char *) arena_alloc(&textInputArena, searchBuffer.capacity);
    searchBuffer.input[0] = '\0';

    Editor editor = {
        .title = "Ravi Editor",
        .width = 1920,
        .height = 1200,
        .codepointsCountASCII = 0,
        .codepointsCountGeo = 0,
        .fontSize = 40,
        .codepointSize = 0,
        .frameCounter = 0,
        .scrollOffset = 0.0,
        .gutterWidth = 150.0,
        .isSearchingCursor = false,
        .editorMode = MODE_NORMAL,
        .mouseOnText = false,
        .isUpdateNeeded = true,
        .textBuffer = &textBuffer,
        .searchBuffer = &searchBuffer,
        .historyBuffer = {
            .actions = {0},
            .current = 0,
            .head = 0,
            .tail = 0,
        },
        .gutter = {
            .x = 0, .y = 0, .width = 0, .height = 0
        },
        .textBox = {
            .x = 0, .y = 0, .width = 0, .height = 0
        },
    };
    editor.currentFilePath[0] = '\0';
    

    //add ascii unicode character range to codepoints
    for (int i = 32; i < 127; i++) editor.codepointsASCII[editor.codepointsCountASCII++] = i;

    //add georgian unicode character range to codepoints
    for (int i = 0x10A0; i < 0x10FF; i++) editor.codepointsGeo[editor.codepointsCountGeo++] = i;


    SetTargetFPS(60);
    Clay_Raylib_Initialize(editor.width, editor.height, editor.title, FLAG_WINDOW_RESIZABLE);
    SetExitKey(0);

    Image icon = LoadImageFromMemory(".png", edit_png, edit_png_len);
    SetWindowIcon(icon);

    uint64_t clayRequiredMemory = Clay_MinMemorySize();
    Clay_Arena clayMemory = (Clay_Arena) {
        .memory = malloc(clayRequiredMemory),
        .capacity = clayRequiredMemory,
    };

    Clay_Initialize(clayMemory, (Clay_Dimensions) {.width = GetScreenWidth(), .height = GetScreenHeight()}, (Clay_ErrorHandler) {0});

    HandleFontLoad(&editor.font, FiraCode_Regular_ttf, FiraCode_Regular_ttf_len, editor.fontSize, editor.codepointsASCII, editor.codepointsCountASCII);
    HandleFontLoad(&editor.gutterFont, FiraCode_Regular_ttf, FiraCode_Regular_ttf_len, editor.fontSize, editor.codepointsASCII, editor.codepointsCountASCII);
    HandleFontLoad(&editor.fallbackFont, NotoSansGeorgian_Regular_ttf, NotoSansGeorgian_Regular_ttf_len, editor.fontSize, editor.codepointsGeo, editor.codepointsCountGeo);

    if (argc > 1) {
        LoadFileInEditor(argv[1], &editor);
    }


    bool exit = false;
    while(!exit) {
        if (WindowShouldClose()) {
            if (editor.textBuffer->isSaved) {
                exit = true;
            } else {
                editor.editorMode = MODE_PROMPT;
            }
        }

        int cursor = MOUSE_CURSOR_DEFAULT;

        Font fonts[3] = {editor.font, editor.fallbackFont, editor.gutterFont};
        Clay_Vector2 mousePosition = {.x = GetMouseX(), .y = GetMouseY()}; 
        Clay_SetPointerState(mousePosition, true);


        Clay_BeginLayout();

        Clay_SetLayoutDimensions((Clay_Dimensions) {.width = GetScreenWidth(), .height = GetScreenHeight()});


        CLAY(CLAY_ID("MainContainer"), {
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM, 
                .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}
            }, 
        }) {

            CLAY(CLAY_ID("TopMenu"), {
                .layout = {
                    .layoutDirection = CLAY_LEFT_TO_RIGHT, 
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(50)}
                }, 
                .backgroundColor = DARK_GRAY_CLAY
                 }) {

                if (ButtonComponent(CLAY_STRING("Open File"), CLAY_STRING("FileOpen"), &cursor, 150.0, (Clay_Color)DARK_GRAY_CLAY)) {
                    HandleFileOpen(&editor);
                };
                if (ButtonComponent(CLAY_STRING("Save File"), CLAY_STRING("FileSave"), &cursor, 150.0, (Clay_Color)DARK_GRAY_CLAY)) {
                    HandleFileSave(&editor);
                }
            }

            CLAY(CLAY_ID("EditorArea"), {
                .layout = {
                    .layoutDirection = CLAY_LEFT_TO_RIGHT, 
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}
                }
            }) {

                CLAY(CLAY_ID("Gutter"), {
                    .layout = {
                        .layoutDirection = CLAY_TOP_TO_BOTTOM, 
                        .sizing = {.width = CLAY_SIZING_FIXED(editor.gutterWidth), .height = CLAY_SIZING_GROW(0)}, 
                        .padding = CLAY_PADDING_ALL(16), 
                        .childGap = 16
                    }, 
                }){}

                CLAY(CLAY_ID("TextBoxContainer"), {
                    .layout = {
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}
                    }
                }) {

                    CLAY(CLAY_ID("TextBox"), {
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}
                        }, 
                        })
                    {}

                    if (editor.editorMode == MODE_SEARCH) {
                        CLAY(CLAY_ID("SearchBox"), {
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(50)},
                                .padding = CLAY_PADDING_ALL(16)}, 
                             })
                        {}
                    }

                }

                CLAY(CLAY_ID("ScrollBar"), {.layout = {.sizing = {.width = CLAY_SIZING_FIXED(40), .height = CLAY_SIZING_GROW(0)},
                     .padding = CLAY_PADDING_ALL(10)},
                     //.backgroundColor = GUTTER_COLOR_CLAY
                     }){
                    if (Clay_Hovered()) {
                        cursor = MOUSE_CURSOR_POINTING_HAND;
                    }
                }

                if (editor.editorMode == MODE_PROMPT) {
                    CLAY(CLAY_ID("PromptBox"), {
                        .floating = {
                        .attachTo = CLAY_ATTACH_TO_PARENT,
                        .zIndex = 100,
                        .attachPoints = {
                                .element = CLAY_ATTACH_POINT_CENTER_CENTER, 
                                .parent = CLAY_ATTACH_POINT_CENTER_CENTER,
                            }
                        }, 
                         .layout = {
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            .sizing = {.width = CLAY_SIZING_FIXED(500), .height = CLAY_SIZING_FIXED(200)},
                            .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                            .childGap = 50,
                         },
                         .backgroundColor = LIGHT_GRAY_CLAY
                         })
                    {
                        if (Clay_Hovered()) {
                            cursor = MOUSE_CURSOR_ARROW;
                        }
                        CLAY(CLAY_ID("PromptTextWrapper"), {
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_FIXED(460), .height = CLAY_SIZING_FIT(0)},
                            }
                        }) {
                             CLAY_TEXT(
                             CLAY_STRING("Do you want to save?"), 
                             CLAY_TEXT_CONFIG({
                                .fontSize = 32, 
                                .textColor = DARK_GRAY_CLAY, 
                                .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                                .wrapMode = true,
                             })
                             );
                        }

                        CLAY(CLAY_ID("PromptButtons"), {
                            .layout = {
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                            .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIXED(50)},
                            .childGap = 20
                        }}) {
                            if (ButtonComponent(CLAY_STRING("Save"), CLAY_STRING("SaveBtn"), &cursor, 130.0, (Clay_Color)COLOR_BLUE)){
                                HandleFileSave(&editor);
                                exit = true;
                            }
                            if (ButtonComponent(CLAY_STRING("Discard"), CLAY_STRING("DiscardBtn"), &cursor, 130.0, (Clay_Color)DARK_GRAY_CLAY)){
                                exit = true;
                            }
                            if (ButtonComponent(CLAY_STRING("Cancel"), CLAY_STRING("CancelBtn"), &cursor, 130.0, (Clay_Color)COLOR_ORANGE)) {
                                editor.editorMode = MODE_NORMAL;
                            }
                        }
                    }
                }
            }

        };

        Clay_RenderCommandArray renderCommands = Clay_EndLayout();

        Clay_ElementData textBoxData =  Clay_GetElementData(Clay_GetElementId(CLAY_STRING("TextBox")));
        Clay_ElementData searchBoxData =  Clay_GetElementData(Clay_GetElementId(CLAY_STRING("SearchBox")));
        Clay_ElementData gutterData =  Clay_GetElementData(Clay_GetElementId(CLAY_STRING("Gutter")));
        Clay_ElementData scrollBarData = Clay_GetElementData(Clay_GetElementId(CLAY_STRING("ScrollBar")));

        editor.textBox.x = textBoxData.boundingBox.x;
        editor.textBox.y = textBoxData.boundingBox.y;
        editor.textBox.width = textBoxData.boundingBox.width;
        editor.textBox.height = textBoxData.boundingBox.height;

        editor.searchBox.x = searchBoxData.boundingBox.x;
        editor.searchBox.y = searchBoxData.boundingBox.y;
        editor.searchBox.width = searchBoxData.boundingBox.width;
        editor.searchBox.height = searchBoxData.boundingBox.height;

        editor.gutter.x = gutterData.boundingBox.x;
        editor.gutter.y = gutterData.boundingBox.y;
        editor.gutter.width = gutterData.boundingBox.width;
        editor.gutter.height = gutterData.boundingBox.height;

        UpdateEditor(&editor, scrollBarData);

        if (cursor == MOUSE_CURSOR_DEFAULT && editor.mouseOnText) {
            cursor = MOUSE_CURSOR_IBEAM;
        }

        SetMouseCursor(cursor);

        if (IsWindowResized()) {
            editor.isUpdateNeeded = true;
        }

        if (editor.isUpdateNeeded) {
            scratchArena.used = 0;
            editor.syntaxTokens.items = (syntax_token_t *) arena_alloc(&scratchArena, (textBuffer.size + 1) * (sizeof(syntax_token_t)));
            editor.syntaxTokens.count = 0;

            textBuffer.rowList.items = (int *) arena_alloc(&scratchArena, (textBuffer.size + 1) * sizeof(int));
            textBuffer.rowList.count = 0;

            calculate_syntax_highlights(&textBuffer, keywords, &editor.syntaxTokens);
            UpdateTextLayout(&editor);
            editor.isUpdateNeeded = false;
        }
        UpdateCursorPosition(&editor);
        HandleScroll(&editor);
        HandleFileDrop(&editor);

        BeginDrawing();
        ClearBackground((Color)DARK_GRAY_CLAY);
        RenderText(&editor);
        if (editor.editorMode == MODE_SEARCH) {
            RenderSearchBarText(&editor);
        }
        RenderScrollbar(&editor, scrollBarData);

        Clay_Raylib_Render(renderCommands, fonts);


        if (!IsWindowFocused()) {
            long fileModTime = GetFileModTime(editor.currentFilePath);
            char currentFilePath[1024] = {0};
            if (editor.currentFilePath[0] != '\0' && fileModTime > editor.lastModificationTime) {
                strncpy(currentFilePath, editor.currentFilePath, 1024);
                ClearEditor(&editor);
                LoadFileInEditor(currentFilePath, &editor);
                editor.lastModificationTime = fileModTime;
            }
        }

        if (editor.currentFilePath[0] != '\0') {
            const char* fileName = GetFileName(editor.currentFilePath);
            editor.textBuffer->isSaved ? SetWindowTitle(fileName): SetWindowTitle(TextFormat("%s *", fileName));
        } else {
            editor.textBuffer->isSaved ? SetWindowTitle("Ravi Editor") : SetWindowTitle("Ravi Editor *");
        }


        EndDrawing();
    }

    UnloadFont(editor.font);
    UnloadFont(editor.fallbackFont);
    UnloadFont(editor.gutterFont);
    UnloadImage(icon);
    CloseWindow();

    arena_free(&textInputArena);
    arena_free(&scratchArena);

    return 0;
}

void UpdateEditor(Editor* editor, Clay_ElementData elementData) {
    HandleKeyboardInput(editor);
    HandleMouseEvents(editor, elementData);

    editor->frameCounter++;
    
}

void DrawCursor(Editor *editor, Vector2 position, Color color) {
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

void RenderText(Editor *editor) {
    text_buffer_t *buffer = editor->textBuffer;

    Vector2 cursorScreenPosition = (Vector2) {
        .x = editor->cursorPosition.x + editor->textBox.x,
        .y = editor->cursorPosition.y + editor->textBox.y - editor->scrollOffset
    };

    if (editor->editorMode == MODE_NORMAL) {
        DrawCursor(editor, cursorScreenPosition, WHITE);
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
            Font *font = GetFontForCodepoint(editor, nextCodePoint);

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

    RenderGutter(editor);
}

void RenderSearchBarText(Editor *editor) {
    DrawRectangleRec(editor->searchBox, LIGHTGRAY);

    int currentX = editor->searchBox.x;
    int i = 0;

    BeginScissorMode((int)editor->searchBox.x, (int)editor->searchBox.y, (int)editor->searchBox.width, (int)editor->searchBox.height);
    while (true) {
        char *ptr = editor->searchBuffer->input + i;
        if (*ptr == '\0') break;
        int nextCodePoint = GetCodepointNext(ptr, &editor->codepointSize);
        Font *font = GetFontForCodepoint(editor, nextCodePoint);

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

    DrawCursor(editor, cursorPosition, BLACK);

}

void RenderGutter(Editor *editor) {
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

void HandleScroll(Editor *editor) {
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

void HandleKeyboardInput(Editor *editor) {
    switch (editor->editorMode) {
        case MODE_NORMAL:
            HandleNormalMode(editor);
        break;
        case MODE_SEARCH:
            HandleSearchMode(editor);
        break;
        case MODE_PROMPT:
            HandlePromptMode(editor);
        break;
    }
}
void HandleNormalMode(Editor *editor) {

    // get user input
    int key = GetCharPressed();
    while (key > 0) {
        int start = min(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
        int end = max(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
        if (start != end) {
            RecordDeleteAction(editor, start, (end - start));
            buffer_delete_range(editor->textBuffer, start, end);
        }

        int byteSize = 0;
        const char *utf8Symbol = buffer_codepoint_to_utf8(key, &byteSize);
        RecordInsertAction(editor, utf8Symbol, byteSize);

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
        RecordInsertAction(editor, "\n", 1);
        buffer_insert_character(editor->textBuffer, '\n');
        editor->isUpdateNeeded = true;
    }

    if (IsKeyPressed(KEY_TAB) || IsKeyPressedRepeat(KEY_TAB)) {
        editor->isSearchingCursor = true;
        RecordInsertAction(editor, "\t", 1);
        buffer_insert_character(editor->textBuffer, '\t');
        editor->isUpdateNeeded = true;
    }

    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        editor->isSearchingCursor = true;
        if (editor->textBuffer->cursorByteOffset != editor->textBuffer->selectionAnchor) {
            int start = min(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
            int end = max(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);

            RecordDeleteAction(editor, start, (end - start));

            buffer_delete_range(editor->textBuffer, start, end);
            editor->isUpdateNeeded = true;
        } else {
            int length = buffer_get_previous_char_size(editor->textBuffer->input, editor->textBuffer->cursorByteOffset);
            RecordDeleteAction(editor, (editor->textBuffer->cursorByteOffset - length), length);
            buffer_delete_character(editor->textBuffer);
            editor->isUpdateNeeded = true;
        }
    }

    if (IsKeyPressed(KEY_DELETE) || IsKeyPressedRepeat(KEY_DELETE)) {
        editor->isSearchingCursor = true;
        if (editor->textBuffer->cursorByteOffset != editor->textBuffer->selectionAnchor) {
            int start = min(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
            int end = max(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);

            RecordDeleteAction(editor, start, (end - start));

            buffer_delete_range(editor->textBuffer, start, end);
            editor->isUpdateNeeded = true;
        } else {
            if (editor->textBuffer->cursorByteOffset >= (int) editor->textBuffer->size) return;

            int byteSize = 0;
            GetCodepoint(&editor->textBuffer->input[editor->textBuffer->cursorByteOffset], &byteSize);

            RecordDeleteAction(editor, (editor->textBuffer->cursorByteOffset - byteSize), byteSize);

            buffer_delete_range(editor->textBuffer, editor->textBuffer->cursorByteOffset, editor->textBuffer->cursorByteOffset + byteSize);
            editor->isUpdateNeeded = true;
        }
    }

    // copy
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_C)) {
        int start = min(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
        int end = max(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);

        Copy(editor, start, end);
    }

    // paste
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V)) {
        const char *clipboardText = GetClipboardText();

        if (clipboardText != NULL) {
            size_t pastedSize = strlen(clipboardText);
            RecordInsertAction(editor, clipboardText, pastedSize);
            buffer_insert_bytes(editor->textBuffer, clipboardText, pastedSize);
            editor->isUpdateNeeded = true;
        }
    }

    // cut
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_X)) {
        int start = min(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);
        int end = max(editor->textBuffer->cursorByteOffset, editor->textBuffer->selectionAnchor);

        Copy(editor, start, end);

        if (start != end) {
            RecordDeleteAction(editor, start, (end - start));
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
        HandleFileSave(editor);
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) {
        HandleFileOpen(editor);
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_EQUAL)) {
        Zoom(editor, 2.0);
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_MINUS)) {
        Zoom(editor, -2.0);
    }

}
void HandleSearchMode(Editor *editor) {
    // get user input
    int key = GetCharPressed();
    while (key > 0) {
        int byteSize = 0;
        const char *utf8Symbol = buffer_codepoint_to_utf8(key, &byteSize);
        buffer_insert_bytes(editor->searchBuffer, utf8Symbol, byteSize);
        FindNextMatch(editor, 0);
        editor->isUpdateNeeded = true;
        key = GetCharPressed();
    }


    if (IsKeyPressed(KEY_ESCAPE)) {
        editor->editorMode = MODE_NORMAL;
    }

    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        buffer_delete_character(editor->searchBuffer);
        FindNextMatch(editor, 0);
        editor->isUpdateNeeded = true;
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressedRepeat(KEY_ENTER)) {
        FindNextMatch(editor, editor->textBuffer->selectionAnchor);
    }
}

void HandlePromptMode(Editor *editor) {
    if(IsKeyPressed(KEY_ESCAPE)) {
        editor->editorMode = MODE_NORMAL;
    }
}

void HandleMouseEvents(Editor *editor, Clay_ElementData elementData) {
    Rectangle rec = (Rectangle) {
        .x = elementData.boundingBox.x, 
        .y = elementData.boundingBox.y,
        .height = elementData.boundingBox.height,
        .width = elementData.boundingBox.width
    };
    bool isMouseOnScrollbar = CheckCollisionPointRec(GetMousePosition(), rec);

    if (CheckCollisionPointRec(GetMousePosition(), editor->textBox)) {
        editor->mouseOnText = true;
    } else {
        editor->mouseOnText = false;
    }

    float mouseWheelMove = GetMouseWheelMove();
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        Zoom(editor, mouseWheelMove);
    } else {
        Scroll(editor, mouseWheelMove);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        editor->isSearchingCursor = false;
        if (isMouseOnScrollbar) {
            editor->isScrolling = true;
        } else {
            editor->isScrolling = false;
            int index = GetIndexFromMouse(editor, GetMouseX(), GetMouseY());
            editor->textBuffer->cursorByteOffset = index;
            editor->textBuffer->selectionAnchor = index;
        }
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (editor->isScrolling) {
            float relativePos = GetMouseY() - elementData.boundingBox.y;
            float percentage = relativePos / elementData.boundingBox.height;
            float maxScroll = max((editor->totalContentHeight - editor->textBox.height), 0);
            float targetScroll = max(editor->totalContentHeight * percentage, 0);
            editor->scrollOffset = min(targetScroll, maxScroll);
        } else {
            int index = GetIndexFromMouse(editor, GetMouseX(), GetMouseY());
            editor->textBuffer->cursorByteOffset = index;
        }
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        editor->isScrolling = false;
    }
}

int GetIndexFromMouse(Editor *editor, int mouseX, int mouseY) {
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
        Font *font = GetFontForCodepoint(editor, codePoint);

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

int min(int x, int y) {
    if (x > y) return y;
    return x;
}

int max(int x, int y) {
    if (x < y) return y;
    return x;
}

void UpdateTextLayout(Editor *editor) {
    editor->textBuffer->rowList.count = 0;
    buffer_push_to_row_list(&editor->textBuffer->rowList, 0);

    char *ptr = editor->textBuffer->input;
    bool running = true;

    Vector2 layoutCursor = {.x = TEXT_OFFSET_X, .y = TEXT_OFFSET_Y};

    while (running) {
        int nextCodePoint = GetCodepointNext(ptr, &editor->codepointSize);
        Font *font = GetFontForCodepoint(editor, nextCodePoint);

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

void HandleFileOpen(Editor *editor) {
    char *fileName = tinyfd_openFileDialog("Open file", "", 0, NULL, NULL, 0);
    if (fileName != NULL) {
        ClearEditor(editor);
        LoadFileInEditor(fileName, editor);
    }
}

void HandleFileDrop(Editor *editor) {
    FilePathList filePathList = {0};
    if (IsFileDropped()) {
        filePathList = LoadDroppedFiles();
        
        if (filePathList.count > 0) {
            const char *fileName = filePathList.paths[0];
            ClearEditor(editor);
            LoadFileInEditor(fileName, editor);
        }

        UnloadDroppedFiles(filePathList);
    }
}

void LoadFileInEditor(const char *fileName, Editor *editor) {
    int size = 0;
    unsigned char *fileData = LoadFileData(fileName, &size);

    if (fileData != NULL) {
        buffer_insert_bytes(editor->textBuffer, (char *) fileData, size);
        strncpy(editor->currentFilePath, fileName, 1024);
        UnloadFileData(fileData);
        editor->lastModificationTime = GetFileModTime(fileName);
        editor->isUpdateNeeded = true;
    }
}

void ClearEditor(Editor *editor) {
    editor->textBuffer->input[0] = '\0';
    editor->currentFilePath[0] = '\0';

    editor->textBuffer->size = 0;
    editor->textBuffer->rowList.count = 0;
    editor->textBuffer->cursorByteOffset = 0;
    editor->textBuffer->selectionAnchor = 0;
    editor->scrollOffset = 0;

    editor->totalContentHeight = 0;

    editor->textBuffer->isSaved = true;
    editor->isUpdateNeeded = true;
}

Font* GetFontForCodepoint(Editor *editor, int codepoint) {
    if (codepoint >= 0x10A0 && codepoint <= 0x10FF) {
        return &editor->fallbackFont;
    }

    return &editor->font;
}

bool ButtonComponent(Clay_String text, Clay_String id, int *cursor, float width, Clay_Color color) {
    bool clicked = false;
    CLAY(CLAY_SID(id), {.layout = {.sizing = {.width = CLAY_SIZING_FIXED(width), .height = CLAY_SIZING_GROW(0)},
         .padding = CLAY_PADDING_ALL(10)},
         .backgroundColor = color}) {
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

void RenderScrollbar(Editor *editor, Clay_ElementData elementData) {
    Rectangle scrollBg = {
        .x = elementData.boundingBox.x, 
        .y = elementData.boundingBox.y, 
        .width = elementData.boundingBox.width, 
        .height = elementData.boundingBox.height
    };
    DrawRectangleRec(scrollBg, GUTTER_COLOR);

    if (!elementData.found) return;

    if (editor->totalContentHeight > editor->textBox.height) {
        float height =max((editor->textBox.height / editor->totalContentHeight) * elementData.boundingBox.height, 10);
        Rectangle rec = (Rectangle) {
            .height = height,
            .width = elementData.boundingBox.width,
            .x = elementData.boundingBox.x,
            .y = elementData.boundingBox.y + (editor->scrollOffset / editor->totalContentHeight * elementData.boundingBox.height)
        };

        DrawRectangleRounded(rec, 0.5, 1, WHITE);
    }
}

bool IsSeparator(int codepoint) {
    static bool table[256] = {false};
    static bool initialized = false;

    if (!initialized) {
        table[' '] = true;
        table['\n'] = true;
        table['\t'] = true;
        table['{'] = true;
        table['}'] = true;
        table['['] = true;
        table[']'] = true;
        table['('] = true;
        table[')'] = true;
        table[';'] = true;
        table[','] = true;
        table['.'] = true;
        table['\''] = true;
        table['\"'] = true;
        table['='] = true;
        table['-'] = true;
        table['+'] = true;
        table['*'] = true;
        table['/'] = true;
        table['&'] = true;
        table['|'] = true;

        initialized = true;
    }

    if (codepoint >= 0 && codepoint < 256) {
        return table[codepoint];
    }

    return false;
}

void HandleFileSave(Editor *editor) {
    char *targetPath = NULL;
    if (editor->currentFilePath[0] == '\0' || editor->lastModificationTime != GetFileModTime(editor->currentFilePath)) {
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
            editor->lastModificationTime = GetFileModTime(targetPath);
        }
    }
}

void Copy(Editor *editor, int start, int end) {
    if (start != end) {
        size_t size = end - start;

        char *highlightedText = (char *)malloc(size + 1);
        memcpy(highlightedText, &editor->textBuffer->input[start], size);

        highlightedText[size] = '\0';

        SetClipboardText(highlightedText);
        free(highlightedText);
    }
}

void UpdateCursorPosition(Editor *editor) {
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
            Font *font = GetFontForCodepoint(editor, codepoint);
            int glyphIndex = GetGlyphIndex(*font, codepoint);
            currentX += font->glyphs[glyphIndex].advanceX;
        }
        j += codepointSize;
    }
    editor->cursorPosition.x = currentX;
}


void HandleFontLoad(Font *currentFont, const unsigned char *fileData, int dataSize, int fontSize, int *codepoints, int codepointCount) {
    if (IsFontValid(*currentFont)) {
        UnloadFont(*currentFont);
    }
    *currentFont = LoadFontFromMemory(".ttf", fileData, dataSize, fontSize, codepoints, codepointCount);
    SetTextureFilter(currentFont->texture, TEXTURE_FILTER_BILINEAR);
}

void RecordDeleteAction(Editor *editor, int startOffset, int length) {
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

void RecordInsertAction(Editor *editor, const char *text, int length) {
    action_t action = {
        .type = ACTION_INSERT,
        .length = length,
        .offset = editor->textBuffer->cursorByteOffset,
        .text = (char *)text,
    };
    record_action(&editor->historyBuffer, action);
}

void FindNextMatch(Editor *editor, int startOffset) {
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
                FindNextMatch(editor, 0);
            }
        }
    } else {
        editor->textBuffer->selectionAnchor = editor->textBuffer->cursorByteOffset;
    }
}

void Zoom(Editor *editor, float zoomAmt) {
    if (zoomAmt!= 0) {
        int oldFontSize = editor->fontSize;
        int sizeIncrement = editor->fontSize + (int)(zoomAmt * 3);
        int newFontSize = min(max(40, sizeIncrement), 120);
        editor->fontSize = newFontSize;
        float scale = 1.0 * newFontSize / oldFontSize;

        if (newFontSize != oldFontSize) {
            HandleFontLoad(&editor->font, FiraCode_Regular_ttf, FiraCode_Regular_ttf_len, editor->fontSize, editor->codepointsASCII, editor->codepointsCountASCII);
            HandleFontLoad(&editor->gutterFont, FiraCode_Regular_ttf, FiraCode_Regular_ttf_len, editor->fontSize, editor->codepointsASCII, editor->codepointsCountASCII);
            HandleFontLoad(&editor->fallbackFont, NotoSansGeorgian_Regular_ttf, NotoSansGeorgian_Regular_ttf_len, editor->fontSize,editor->codepointsGeo, editor->codepointsCountGeo);
            editor->isUpdateNeeded = true;
            editor->scrollOffset *= scale;
            editor->isSearchingCursor = true;
        }
    }
}

void Scroll(Editor *editor, float scrollAmt) {
    if (scrollAmt != 0) {
        editor->scrollOffset -= scrollAmt * editor->fontSize;
        editor->isSearchingCursor = false;
    }
}
