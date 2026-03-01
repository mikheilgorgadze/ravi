#include <assert.h>
#include <raylib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "arena.h"
#include "editor.h"
#include "file_io.h"
#include "render.h"
#include "ui.h"
#include "../resources/fonts/firacode.h"
#include "../resources/fonts/notosans.h"
#include "../resources/logo.h"

#define DARK_GRAY CLITERAL(Color) {48, 48, 48, 255}

#define MAX_INPUT_CHAR 100000000
#define TEXT_ARENA_SIZE 1024 * 1024 * 1024

#define FONT_SANS "resources/fonts/NotoSansGeorgian-Regular.ttf"
#define FONT_MONO "resources/fonts/FiraCode-Regular.ttf"

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


int main(int argc, char *argv[]) {

    arena_t textInputArena = {0};
    arena_initialize(&textInputArena, TEXT_ARENA_SIZE / 2);

    arena_t scratchArena = {0};
    arena_initialize(&scratchArena, TEXT_ARENA_SIZE / 2);

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

    editor_t editor = {
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
    SetExitKey(0);

    Image icon = LoadImageFromMemory(".png", edit_png, edit_png_len);
    SetWindowIcon(icon);

    ui_initialize(editor.width, editor.height, editor.title);

    editor_handle_font_load(&editor.font, FiraCode_Regular_ttf, FiraCode_Regular_ttf_len, editor.fontSize, editor.codepointsASCII, editor.codepointsCountASCII);
    editor_handle_font_load(&editor.gutterFont, FiraCode_Regular_ttf, FiraCode_Regular_ttf_len, editor.fontSize, editor.codepointsASCII, editor.codepointsCountASCII);
    editor_handle_font_load(&editor.fallbackFont, NotoSansGeorgian_Regular_ttf, NotoSansGeorgian_Regular_ttf_len, editor.fontSize, editor.codepointsGeo, editor.codepointsCountGeo);

    if (argc > 1) {
        fileio_load_file_in_editor(argv[1], &editor);
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
        Rectangle scrollBarRec = {0};
        bool scrollBarFound = false;

        ui_build_layout(&editor, &cursor, &exit, &scrollBarRec, &scrollBarFound);

        editor_update(&editor, &scrollBarRec);

        if (cursor == MOUSE_CURSOR_DEFAULT && editor.mouseOnText) {
            cursor = MOUSE_CURSOR_IBEAM;
        }

        SetMouseCursor(cursor);

        if (IsWindowResized()) {
            editor.isUpdateNeeded = true;
        }

        if (editor.fontsNeedReload) {
            editor_handle_font_load(&editor.font, FiraCode_Regular_ttf, FiraCode_Regular_ttf_len, editor.fontSize, editor.codepointsASCII, editor.codepointsCountASCII);
            editor_handle_font_load(&editor.gutterFont, FiraCode_Regular_ttf, FiraCode_Regular_ttf_len, editor.fontSize, editor.codepointsASCII, editor.codepointsCountASCII);
            editor_handle_font_load(&editor.fallbackFont, NotoSansGeorgian_Regular_ttf, NotoSansGeorgian_Regular_ttf_len, editor.fontSize,editor.codepointsGeo, editor.codepointsCountGeo);
            editor.fontsNeedReload = false;
        }

        if (editor.isUpdateNeeded) {
            scratchArena.current->used = 0;
            editor.syntaxTokens.items = (syntax_token_t *) arena_alloc(&scratchArena, (textBuffer.size + 1) * (sizeof(syntax_token_t)));
            editor.syntaxTokens.count = 0;

            textBuffer.rowList.items = (int *) arena_alloc(&scratchArena, (textBuffer.size + 1) * sizeof(int));
            textBuffer.rowList.count = 0;

            calculate_syntax_highlights(&textBuffer, keywords, &editor.syntaxTokens);
            editor_update_text_layout(&editor);
            editor.isUpdateNeeded = false;
        }

        editor_update_cursor_position(&editor);
        editor_handle_scroll(&editor);
        fileio_handle_file_drop(&editor);

        //Raylib begin drawing
        BeginDrawing();

        ClearBackground((Color)DARK_GRAY_CLAY);
        render_text(&editor);
        if (editor.editorMode == MODE_SEARCH) {
            render_search_bar_text(&editor);
        }

        render_scroll_bar(&editor, scrollBarRec, scrollBarFound);

        if (!IsWindowFocused()) {
            long fileModTime = GetFileModTime(editor.currentFilePath);
            char currentFilePath[1024] = {0};
            if (editor.currentFilePath[0] != '\0' && fileModTime > editor.lastModificationTime) {
                strncpy(currentFilePath, editor.currentFilePath, 1024);
                fileio_clear_editor(&editor);
                fileio_load_file_in_editor(currentFilePath, &editor);
                editor.lastModificationTime = fileModTime;
            }
        }

        Font fonts[3] = {editor.font, editor.fallbackFont, editor.gutterFont};
        ui_render(fonts);

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

