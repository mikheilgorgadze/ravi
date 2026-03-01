#include "ui.h"
#include "../lib/clay.h"
#include "file_io.h"

#define CLAY_IMPLEMENTATION
#include "../renderer/clay_renderer_raylib.h"
#include <raylib.h>

static Clay_RenderCommandArray renderCommands;

static bool ButtonComponent(Clay_String text, Clay_String id, int *cursor, float width, Clay_Color color) {
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

void ui_initialize(int width, int height, const char *title) {
    Clay_Raylib_Initialize(width, height, title, FLAG_WINDOW_RESIZABLE);

    uint64_t clayRequiredMemory = Clay_MinMemorySize();
    Clay_Arena clayMemory = (Clay_Arena) {
        .memory = malloc(clayRequiredMemory),
        .capacity = clayRequiredMemory,
    };

    Clay_Initialize(clayMemory, (Clay_Dimensions) {.width = GetScreenWidth(), .height = GetScreenHeight()}, (Clay_ErrorHandler) {0});
}


void ui_build_layout(editor_t *editor, int *cursor, bool *exitFlag, Rectangle *scrollBarRec, bool *scrollBarFound){
        Clay_SetLayoutDimensions((Clay_Dimensions) {.width = (float) GetScreenWidth(), .height = (float) GetScreenHeight()});

        Clay_Vector2 mousePosition = {.x = (float) GetMouseX(), .y = (float) GetMouseY()}; 
        Clay_SetPointerState(mousePosition, IsMouseButtonDown(MOUSE_BUTTON_LEFT));

        Clay_BeginLayout();

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

                if (ButtonComponent(CLAY_STRING("Open File"), CLAY_STRING("FileOpen"), cursor, 150.0, (Clay_Color)DARK_GRAY_CLAY)) {
                    fileio_handle_file_open(editor);
                };
                if (ButtonComponent(CLAY_STRING("Save File"), CLAY_STRING("FileSave"), cursor, 150.0, (Clay_Color)DARK_GRAY_CLAY)) {
                    filio_handle_file_save(editor);
                }
            }

            CLAY(CLAY_ID("editor_tArea"), {
                .layout = {
                    .layoutDirection = CLAY_LEFT_TO_RIGHT, 
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}
                }
            }) {

                CLAY(CLAY_ID("Gutter"), {
                    .layout = {
                        .layoutDirection = CLAY_TOP_TO_BOTTOM, 
                        .sizing = {.width = CLAY_SIZING_FIXED(editor->gutterWidth), .height = CLAY_SIZING_GROW(0)}, 
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

                    if (editor->editorMode == MODE_SEARCH) {
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
                     }){
                    if (Clay_Hovered()) {
                        *cursor = MOUSE_CURSOR_POINTING_HAND;
                    }
                }

                if (editor->editorMode == MODE_PROMPT) {
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
                            *cursor = MOUSE_CURSOR_ARROW;
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
                            if (ButtonComponent(CLAY_STRING("Save"), CLAY_STRING("SaveBtn"), cursor, 130.0, (Clay_Color)COLOR_BLUE)){
                                filio_handle_file_save(editor);
                                *exitFlag = true;
                            }
                            if (ButtonComponent(CLAY_STRING("Discard"), CLAY_STRING("DiscardBtn"), cursor, 130.0, (Clay_Color)DARK_GRAY_CLAY)){
                                *exitFlag = true;
                            }
                            if (ButtonComponent(CLAY_STRING("Cancel"), CLAY_STRING("CancelBtn"), cursor, 130.0, (Clay_Color)COLOR_ORANGE)) {
                                editor->editorMode = MODE_NORMAL;
                            }
                        }
                    }
                }
            }

        };

        renderCommands = Clay_EndLayout();

        Clay_ElementData textBoxData =  Clay_GetElementData(Clay_GetElementId(CLAY_STRING("TextBox")));
        Clay_ElementData searchBoxData =  Clay_GetElementData(Clay_GetElementId(CLAY_STRING("SearchBox")));
        Clay_ElementData gutterData =  Clay_GetElementData(Clay_GetElementId(CLAY_STRING("Gutter")));
        Clay_ElementData scrollBarData = Clay_GetElementData(Clay_GetElementId(CLAY_STRING("ScrollBar")));

        editor->textBox.x = textBoxData.boundingBox.x;
        editor->textBox.y = textBoxData.boundingBox.y;
        editor->textBox.width = textBoxData.boundingBox.width;
        editor->textBox.height = textBoxData.boundingBox.height;

        editor->searchBox.x = searchBoxData.boundingBox.x;
        editor->searchBox.y = searchBoxData.boundingBox.y;
        editor->searchBox.width = searchBoxData.boundingBox.width;
        editor->searchBox.height = searchBoxData.boundingBox.height;

        editor->gutter.x = gutterData.boundingBox.x;
        editor->gutter.y = gutterData.boundingBox.y;
        editor->gutter.width = gutterData.boundingBox.width;
        editor->gutter.height = gutterData.boundingBox.height;

        *scrollBarRec = (Rectangle) {
            .x      = scrollBarData.boundingBox.x,
            .y      = scrollBarData.boundingBox.y,
            .width  = scrollBarData.boundingBox.width,
            .height = scrollBarData.boundingBox.height,
        };
        *scrollBarFound = scrollBarData.found;
}

void ui_render(Font *fonts) {
    Clay_Raylib_Render(renderCommands, fonts);
}
