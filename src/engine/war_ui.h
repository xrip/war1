#pragma once

#include <stdarg.h>

#include "shl/wstr.h"

#include "war.h"
#include "war_entities.h"

bool wui_isUIEntity(WarEntity* entity);

void wui_clearUIText(WarContext* context, WarEntity* uiText);


void wui_clearUITooltip(WarContext* context, WarEntity* uiButton);





typedef struct
{
    vec2 position;
    vec2 rotation;
    vec2 scale;
    StringView text;
    s32 fontIndex;
    f32 fontSize;
    f32 lineHeight;
    WarColor fontColor;
    WarColor highlightColor;
    s32 highlightIndex;
    s32 highlightCount;
    vec2 boundings;
    WarTextAlignment horizontalAlign;
    WarTextAlignment verticalAlign;
    WarTextAlignment lineAlign;
    WarTextWrapping  wrapping;
    WarTextTrimming  trimming;
    bool multiline;
} CreateUITextArgs;

typedef struct
{
    vec2 position;
    vec2 rotation;
    vec2 scale;
    vec2 size;
    WarColor color;
} CreateUIRectArgs;

typedef struct
{
    WarSpriteResourceRef spriteRef;
    vec2 position;
    vec2 rotation;
    vec2 scale;
} CreateUIImageArgs;

typedef struct
{
    WarCursorType type;
    vec2 position;
    vec2 rotation;
    vec2 scale;
    s32 frameIndex;
} CreateUICursorArgs;

typedef struct
{
    vec2 position;
    vec2 rotation;
    vec2 scale;

    s32 fontIndex;
    f32 fontSize;
    StringView text;
    f32 lineHeight;
    WarColor fontColor;
    WarColor highlightColor;
    s32 highlightIndex;
    s32 highlightCount;
    WarTextAlignment horizontalAlign;
    WarTextAlignment verticalAlign;
    WarTextAlignment lineAlign;
    WarTextWrapping wrapping;
    WarTextTrimming trimming;

    WarSpriteResourceRef backgroundNormalRef;
    WarSpriteResourceRef backgroundPressedRef;
    WarSpriteResourceRef foregroundRef;

    bool interactive;
    WarKeys hotKey;
    StringView tooltip;
    s32 tooltipHighlightIndex;
    s32 tooltipHighlightCount;
    s32 gold;
    s32 wood;
    WarClickHandler clickHandler;
} CreateUITextButtonArgs;

typedef struct
{
    WarSpriteResourceRef backgroundNormalRef;
    WarSpriteResourceRef backgroundPressedRef;
    WarSpriteResourceRef foregroundRef;
    vec2 position;
    vec2 rotation;
    vec2 scale;

    bool interactive;
    WarKeys hotKey;
    StringView tooltip;
    s32 tooltipHighlightIndex;
    s32 tooltipHighlightCount;
    s32 gold;
    s32 wood;
    s32 foregroundFrameIndex;
    WarClickHandler clickHandler;
} CreateUIImageButtonArgs;

#define CREATE_UI_TEXT_ARGS_INIT_CONST(...) { \
    .scale          = {1, 1}, \
    .fontSize       = 10, \
    .fontColor      = FONT_NORMAL_COLOR_INIT, \
    .highlightColor = FONT_HIGHLIGHT_COLOR_INIT, \
    .highlightIndex = NO_HIGHLIGHT, \
    .verticalAlign  = WAR_TEXT_ALIGN_TOP, \
    .lineAlign      = WAR_TEXT_ALIGN_LEFT, \
    .wrapping       = WAR_TEXT_WRAP_NONE, \
    __VA_ARGS__ \
}
#define CREATE_UI_TEXT_ARGS_INIT(...) (&(CreateUITextArgs)CREATE_UI_TEXT_ARGS_INIT_CONST(__VA_ARGS__))

#define CREATE_UI_RECT_ARGS_INIT_CONST(...) { \
    .scale = {1, 1}, \
    __VA_ARGS__ \
}
#define CREATE_UI_RECT_ARGS_INIT(...) (&(CreateUIRectArgs)CREATE_UI_RECT_ARGS_INIT_CONST(__VA_ARGS__))

#define CREATE_UI_IMAGE_ARGS_INIT_CONST(...) { \
    .scale = {1, 1}, \
    __VA_ARGS__ \
}
#define CREATE_UI_IMAGE_ARGS_INIT(...) (&(CreateUIImageArgs)CREATE_UI_IMAGE_ARGS_INIT_CONST(__VA_ARGS__))

#define CREATE_UI_CURSOR_ARGS_INIT_CONST(...) { \
    .scale = {1, 1}, \
    __VA_ARGS__ \
}
#define CREATE_UI_CURSOR_ARGS_INIT(...) (&(CreateUICursorArgs)CREATE_UI_CURSOR_ARGS_INIT_CONST(__VA_ARGS__))

#define CREATE_UI_TEXT_BUTTON_ARGS_INIT_CONST(...) { \
    .scale                 = {1, 1}, \
    .fontSize              = 10, \
    .fontColor             = FONT_NORMAL_COLOR_INIT, \
    .highlightColor        = FONT_HIGHLIGHT_COLOR_INIT, \
    .highlightIndex        = NO_HIGHLIGHT, \
    .foregroundRef         = { -1, 0, {0} }, \
    .interactive           = true, \
    .hotKey                = WAR_KEY_NONE, \
    .tooltipHighlightIndex = NO_HIGHLIGHT, \
    .horizontalAlign       = WAR_TEXT_ALIGN_CENTER, \
    .verticalAlign         = WAR_TEXT_ALIGN_MIDDLE, \
    __VA_ARGS__ \
}
#define CREATE_UI_TEXT_BUTTON_ARGS_INIT(...) (&(CreateUITextButtonArgs)CREATE_UI_TEXT_BUTTON_ARGS_INIT_CONST(__VA_ARGS__))

#define CREATE_UI_IMAGE_BUTTON_ARGS_INIT_CONST(...) { \
    .scale                 = {1, 1}, \
    .foregroundRef         = { -1, 0, {0} }, \
    .interactive           = true, \
    .hotKey                = WAR_KEY_NONE, \
    .tooltipHighlightIndex = NO_HIGHLIGHT, \
    .foregroundFrameIndex  = 0, \
    __VA_ARGS__ \
}
#define CREATE_UI_IMAGE_BUTTON_ARGS_INIT(...) (&(CreateUIImageButtonArgs)CREATE_UI_IMAGE_BUTTON_ARGS_INIT_CONST(__VA_ARGS__))

void wui_changeCursorType(WarContext* context, WarCursorType type);
void wui_updateUIButtons(WarContext* context, bool hotKeysEnabled);

void wui_renderUIEntities(WarContext* context);
