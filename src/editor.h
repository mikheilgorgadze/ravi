#ifndef EDITOR_H
#define EDITOR_H

#include "buffer.h"
#include "lexer.h"
#include "history.h"
#include <raylib.h>

#define TEXT_OFFSET_X 5
#define TEXT_OFFSET_Y 5
#define PADDING 40

typedef enum {
    MODE_NORMAL,
    MODE_SEARCH,
    MODE_PROMPT,
} editor_mode_t;

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

    double lastClickTime;
    int clickCount;

    text_buffer_t *searchBuffer;
    editor_mode_t editorMode;

    int codepointsASCII[256];
    int codepointsCountASCII;

    int codepointsGeo[256];
    int codepointsCountGeo;

    syntax_token_list_t syntaxTokens;
    history_buffer_t historyBuffer;

    arena_t frameArena;

    bool isSearchingCursor;
    bool isScrolling;
    bool mouseOnText;
    bool isUpdateNeeded;
    bool fontsNeedReload;
    bool isAutoSelecting;

    Font font;
    Font gutterFont;
    Font fallbackFont;
    Vector2 cursorPosition;
    text_buffer_t *textBuffer;
    Rectangle gutter;
    Rectangle textBox;
    Rectangle searchBox;
} editor_t;


void editor_record_delete_action(editor_t *editor, int startOffset, int length);
void editor_record_insert_action(editor_t *editor, const char *text, int length);
void editor_copy(editor_t *editor, int start, int end);
void editor_handle_font_load(Font *currentFont, const unsigned char *fileData, int dataSize, int fontSize, int *codepoints, int codepointCount);
void editor_update(editor_t* editor, Rectangle *scrollBarRec);
void editor_handle_scroll(editor_t *editor);
void editor_update_text_layout(editor_t *editor);
void editor_update_cursor_position(editor_t *editor);
Font* editor_get_font_for_codepoint(editor_t *editor, int codepoint);

#endif
