// Utility for simple, good Graphical User Interfaces in Raylib
//
// Define GUI_IMPLEMENTATION in one file
// Define GUI_MALLOC, GUI_FREE to overwrite default malloc and free functions

#ifndef GUI_H_
#define GUI_H_

// @TODO: Move this to a separate repo as its own, standalone, single-header library, since I have been copy-pasting this same code between different projects already

#include "util.h"
#include "raylib.h"
#include <string.h>
#include <stdbool.h>
#include <float.h>

#ifndef STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#endif // STB_IMPLEMENTATION

#if !defined(GUI_MALLOC) && !defined(GUI_FREE)
#include <stdlib.h>
#define GUI_MALLOC(size) malloc(size)
#define GUI_FREE(ptr)    free(ptr)
#elif !defined(GUI_MALLOC) || !defined(GUI_FREE)
#error "You must define both GUI_MALLOC and GUI_FREE, or neither."
#elif !defined(MALLOC)
#define MALLOC(size) GUI_MALLOC(size)
#endif

typedef enum __attribute__((__packed__)) {
    EL_STATE_HIDDEN,   // Element is not displayed
    EL_STATE_INACTIVE, // Element is not active in any way. `state > EL_STATE_INACTIVE` can be used to check if the element is active in anyway
    EL_STATE_HOVERED,  // Element is currently being hovered
    EL_STATE_PRESSED,  // Element is just being clicked on in this frame. It is active from now on
    EL_STATE_FOCUSED,  // Element is being focused on and active. To be active is to accept user input
} Gui_El_State;

typedef enum __attribute__((__packed__)) {
    TEXT_ALIGN_LT, // Align left and/or top
    TEXT_ALIGN_C,  // Align center
    TEXT_ALIGN_RB, // Align right and/or bottom
} Gui_Text_Align;

typedef struct {
    Color bg;           // Color for background
    Color border_color; // Color for border
    i32   border_width; // Width of border
    // The following is only relevant for text
    Color color;     // Color for text
    Font  font;      // The font used
    i32   pad;       // Padding; Space between text and border
    float font_size; // Font Size
    float cSpacing;  // Spacing between characters of text
    float lSpacing;  // Spacing between lines of text
    Gui_Text_Align hAlign; // Horizontal Text Alignment
    Gui_Text_Align vAlign; // Vertical Text Alignment
} Gui_El_Style;

typedef struct {
    // @Note: All coordinates here are absolute and not relative to any bounding box
    const char *text;  // Text to be drawn
    u16  *lineOffsets; // (dynamic array) Amount of chars until the next line should start ('\n' is ignored)
    i32  *lineXs;      // (dynamic array) x coordinates of line starts (y coordinates come from Gui_El_Style)
    i32   y;           // y coordinate of first line
    float w;           // Width of text (Height can be calculated via amount of lines and style)
    u32   text_len;    // Amount of bytes in tet
} Gui_Drawable_Text;

typedef struct {
    Rectangle bounds;
    char *text;
    Gui_El_Style defaultStyle;
    Gui_El_Style hovered;
} Gui_Label;

typedef struct {
    char *placeholder;      // Placeholder
    Gui_Label label;        // Gui_Label to display and update on input
    u32 cur;                // Index in the text at which the cursor should be displayed
    i32 anim_idx;           // Current index in playing animation - if negative, it is currently in waiting time
    u16  rows;              // Amount of rows in label.text. If there's no newline, `rows == 1`
    bool resize;
    bool multiline;
    bool selected;
    // @TODO:
} Gui_Input_Box;

typedef struct {
    bool updated;
    bool enter;
    bool tab;
    Gui_El_State state;
} Gui_Update_Res;

void gui_setTextLineSpacing(i32 spacing);
bool gui_stateIsActive(Gui_El_State state);
bool gui_isPointInRec(i32 px, i32 py, i32 rx, i32 ry, i32 rw, i32 rh);
Gui_El_Style gui_defaultStyle(Font font);
Gui_El_Style gui_cloneStyle(Gui_El_Style self);
Gui_Drawable_Text gui_prepTextForDrawing(const char *text, Rectangle bounds, Gui_El_Style style);
Vector2 gui_measureText(char *text, Rectangle bounds, Gui_El_Style style, Gui_Drawable_Text *drawable_text);
void gui_drawPreparedText(Gui_Drawable_Text text, Gui_El_Style style);
void gui_drawText(const char *text, Rectangle bounds, Gui_El_Style style);
void gui_drawSized(const char *text, Rectangle bounds, Gui_El_Style style);
Vector2* gui_drawSizedEx(Gui_Drawable_Text text, Rectangle bounds, Gui_El_Style style);
Gui_Label gui_newLabel(Rectangle bounds, char *text, Gui_El_Style defaultStyle, Gui_El_Style hovered);
Gui_Label gui_newCenteredLabel(Rectangle bounds, i32 w, char *text, Gui_El_Style defaultStyle, Gui_El_Style hovered);
void gui_centerLabel(Gui_Label *self, Rectangle bounds, i32 w);
void gui_rmCharLabel(Gui_Label *self, u32 idx);
void gui_insertCharLabel(Gui_Label *self, i32 idx, char c);
void gui_insertSliceLabel(Gui_Label *self, i32 idx, const char *slice, i32 slice_size);
Gui_El_State gui_getState(i32 x, i32 y, i32 w, i32 h);
Vector2 gui_measureLabelText(Gui_Label self, Gui_El_State state);
void gui_resizeLabel(Gui_Label *self, Gui_El_State state);
Gui_Drawable_Text gui_resizeLabelEx(Gui_Label *self, Gui_El_State state, char *text);
Gui_El_State gui_drawLabel(Gui_Label self);
Gui_Input_Box gui_newInputBox(char *placeholder, bool resize, bool multiline, bool selected, Gui_Label label);
bool gui_isInputBoxHovered(Gui_Input_Box self);
Gui_El_State gui_getInputBoxState(Gui_Input_Box *self);
Gui_El_State gui_getInputBoxStateHelper(Gui_Input_Box *self, bool hovered);
void gui_resetInputBoxAnim(Gui_Input_Box *self);
Gui_Update_Res gui_handleKeysInputBox(Gui_Input_Box *self);
Gui_Update_Res gui_drawInputBox(Gui_Input_Box *self);

#endif // GUI_H_


#ifdef GUI_IMPLEMENTATION
#ifndef _GUI_IMPL_GUARD_
#define _GUI_IMPL_GUARD_

// Static Variables
static i16   Input_Box_anim_len  = 50;   // Length of animation in ms
static i16   Input_Box_anim_wait = 30;   // Time in ms to wait after the cursor moved before starting to animate again
static i32   Input_Box_cur_width = 4;    // Width of the displayed cursor
static Color Input_Box_cur_color = { 0, 121, 241, 255 }; // Color of the displayed cursor
static i32   text_line_spacing   = 15;   // Same as in raylib;

inline void gui_setTextLineSpacing(i32 spacing)
{
    text_line_spacing = spacing;
    SetTextLineSpacing(spacing);
}

inline bool gui_stateIsActive(Gui_El_State state)
{
    return state >= EL_STATE_PRESSED;
}

inline bool gui_isPointInRec(i32 px, i32 py, i32 rx, i32 ry, i32 rw, i32 rh)
{
    return (px >= rx) && (px <= rx + rw) && (py >= ry) && (py <= ry + rh);
}

Gui_El_Style gui_defaultStyle(Font font)
{
    return (Gui_El_Style) {
        .bg           = BLANK,
        .border_color = BLANK,
        .border_width = 0,
        .color        = WHITE,
        .pad          = 0,
        .font         = font,
        .font_size    = 30,
        .cSpacing     = 0,
        .lSpacing     = 5,
    };
}

Gui_El_Style gui_cloneStyle(Gui_El_Style self)
{
    return (Gui_El_Style) {
        .bg           = self.bg,
        .border_color = self.border_color,
        .border_width = self.border_width,
        .color        = self.color,
        .pad          = self.pad,
        .font         = self.font,
        .font_size    = self.font_size,
        .cSpacing     = self.cSpacing,
        .lSpacing     = self.lSpacing,
    };
}

Gui_Drawable_Text gui_prepTextForDrawing(const char *text, Rectangle bounds, Gui_El_Style style)
{
    if (!text) return (Gui_Drawable_Text) {0};
    float y = bounds.y + style.pad;
    Gui_Drawable_Text out = {0};
    out.text = text;
    out.y    = y;
    stbds_arrsetcap(out.lineOffsets, 16);
    stbds_arrsetcap(out.lineXs, 17);

    float scaleFactor = style.font_size / (float)style.font.baseSize; // Character quad scaling factor
    float lineWidth   = 0.0f;
    u16   lineOffset  = 0;


    i32 cp;        // Current codepoint
    i32 cpSize;    // Current codepoint size in bytes
    float cpWidth; // Width of current codepoint
    for (; (cp = GetCodepointNext(text, &cpSize)); text += cpSize, out.text_len += cpSize, lineOffset += cpSize) {
        if (cp == '\n') {
            stbds_arrput(out.lineOffsets, lineOffset);
            if (lineWidth > out.w) out.w = lineWidth;
            switch (style.hAlign) {
                case TEXT_ALIGN_LT:
                    stbds_arrput(out.lineXs, bounds.x + style.pad);
                    break;
                case TEXT_ALIGN_C:
                    stbds_arrput(out.lineXs, bounds.x + (bounds.width - lineWidth)/2.0f);
                    break;
                case TEXT_ALIGN_RB:
                    stbds_arrput(out.lineXs, bounds.x + bounds.width - lineWidth);
                    break;
            }
            y += style.font_size + style.lSpacing;
            lineOffset = 0;
            lineWidth  = 0;
        }
        else {
            int glyphIndex = GetGlyphIndex(style.font, cp);
            float w = style.font.glyphs[glyphIndex].advanceX ? style.font.glyphs[glyphIndex].advanceX : style.font.recs[glyphIndex].width;
            cpWidth = style.cSpacing + scaleFactor*w;

            // @TODO: When splittin text into new lines, it would be nice to split text by words instead of by characters
            if (lineWidth + cpWidth > bounds.width - style.pad) {
                stbds_arrput(out.lineOffsets, lineOffset - 1);
                if (lineWidth > out.w) out.w = lineWidth;
                switch (style.hAlign) {
                    case TEXT_ALIGN_LT:
                        stbds_arrput(out.lineXs, bounds.x + style.pad);
                        break;
                    case TEXT_ALIGN_C:
                        stbds_arrput(out.lineXs, bounds.x + (bounds.width - lineWidth)/2.0f);
                        break;
                    case TEXT_ALIGN_RB:
                        stbds_arrput(out.lineXs, bounds.x + bounds.width - lineWidth - style.pad);
                        break;
                }
                y += style.font_size + style.lSpacing;
                lineOffset = 1;
                lineWidth  = cpWidth;
            }
            else {
                lineWidth += cpWidth;
            }
        }
    }

    // @Cleanup: Almost identical code with switch-case here 3 times
    if (lineWidth > out.w) out.w = lineWidth;
    switch (style.hAlign) {
        case TEXT_ALIGN_LT:
            stbds_arrput(out.lineXs, bounds.x + style.pad);
            break;
        case TEXT_ALIGN_C:
            stbds_arrput(out.lineXs, bounds.x + (bounds.width - lineWidth)/2.0f);
            break;
        case TEXT_ALIGN_RB:
            stbds_arrput(out.lineXs, bounds.x + bounds.width - lineWidth);
            break;
    }

    // @TODO: Check for case where text is too big for bounding box
    float height = y - out.y;
    if (lineOffset) height += style.font_size;
    switch (style.vAlign) {
        case TEXT_ALIGN_LT:
            break;
        case TEXT_ALIGN_C:
            out.y = bounds.y + (bounds.height - height)/2.0f;
            break;
        case TEXT_ALIGN_RB:
            out.y = bounds.y + bounds.height - height - style.pad;
            break;
    }

    return out;
}

// drawable_text will be written to, except if text == drawable_text.text
Vector2 gui_measureText(char *text, Rectangle bounds, Gui_El_Style style, Gui_Drawable_Text *drawable_text)
{
    if (text != drawable_text->text) *drawable_text = gui_prepTextForDrawing(text, bounds, style);
    i32 lines_count = stbds_arrlen(drawable_text->lineXs);
    float height = lines_count*style.font_size + (lines_count - 1)*style.lSpacing;
    return (Vector2) {
        .x = drawable_text->w,
        .y = height,
    };
}

void gui_drawText(const char *text, Rectangle bounds, Gui_El_Style style)
{
    Gui_Drawable_Text preppedText = gui_prepTextForDrawing(text, bounds, style);
    gui_drawPreparedText(preppedText, style);
}

void gui_drawPreparedText(Gui_Drawable_Text text, Gui_El_Style style)
{
    if (!text.text) return;
    float scaleFactor = style.font_size/style.font.baseSize; // Character quad scaling factor
    Vector2 pos = { .x = text.lineXs[0], .y = text.y }; // Position to draw current codepoint at
    i32 lastOffset = 0;
    i32 cp;        // Current codepoint
    i32 cpSize;    // Current codepoint size in bytes
    for (i32 i = 0, lineIdx = 0, xIdx = 1; (cp = GetCodepointNext(&text.text[i], &cpSize)) != 0; i += cpSize) {
        if ((cp != '\n') && (cp != ' ') && (cp != '\t') && (cp != '\r')) {
            DrawTextCodepoint(style.font, cp, pos, style.font_size, style.color);
        }
        if (UNLIKELY(lineIdx < stbds_arrlen(text.lineOffsets) && i == lastOffset + (i32)text.lineOffsets[lineIdx])) {
            pos.y += style.font_size + style.lSpacing;
            pos.x  = text.lineXs[xIdx];
            xIdx++;
            lineIdx++;
            lastOffset = i;
        } else {
            i32 idx = GetGlyphIndex(style.font, cp);
            float w = style.font.glyphs[idx].advanceX ? style.font.glyphs[idx].advanceX : style.font.recs[idx].width;
            pos.x += style.cSpacing + scaleFactor*w;
        }
    }
}

void gui_drawSized(const char *text, Rectangle bounds, Gui_El_Style style)
{
    if (style.border_width > 0) {
        DrawRectangle(bounds.x - style.border_width, bounds.y - style.border_width, bounds.width + 2*style.border_width, bounds.height + 2*style.border_width, style.border_color);
    }
    DrawRectangle(bounds.x, bounds.y, bounds.width, bounds.height, style.bg);
    gui_drawText(text, bounds, style);
}

/// Same as gui_drawSized, except it returns an array of coordinates for each byte in the drawn text
Vector2* gui_drawSizedEx(Gui_Drawable_Text text, Rectangle bounds, Gui_El_Style style)
{
    // @Cleanup: Almost identical code to gui_drawPreparedText
    if (style.border_width > 0) {
        DrawRectangle(bounds.x - style.border_width, bounds.y - style.border_width, bounds.width + 2*style.border_width, bounds.height + 2*style.border_width, style.border_color);
    }
    DrawRectangle(bounds.x, bounds.y, bounds.width, bounds.height, style.bg);

    if (!text.text) return NULL;
    Vector2 *res = GUI_MALLOC(text.text_len * sizeof(Vector3));
    float scaleFactor = style.font_size/style.font.baseSize; // Character quad scaling factor
    Vector2 pos = { .x = text.lineXs[0], .y = text.y }; // Position to draw current codepoint at
    i32 lastOffset = 0;
    i32 cp;        // Current codepoint
    i32 cpSize;    // Current codepoint size in bytes
    for (i32 i = 0, lineIdx = 0, xIdx = 1; (cp = GetCodepointNext(&text.text[i], &cpSize)) != 0; i += cpSize) {
        for (i32 j = 0; j < cpSize; j++) {
            res[i + j] = pos;
        }
        if ((cp != '\n') && (cp != ' ') && (cp != '\t') && (cp != '\r')) {
            DrawTextCodepoint(style.font, cp, pos, style.font_size, style.color);
        }
        if (UNLIKELY(lineIdx < stbds_arrlen(text.lineOffsets) && i == lastOffset + (i32)text.lineOffsets[lineIdx])) {
            pos.y += style.font_size + style.lSpacing;
            pos.x  = text.lineXs[xIdx];
            xIdx++;
            lineIdx++;
            lastOffset = i;
        } else {
            i32 idx = GetGlyphIndex(style.font, cp);
            float w = style.font.glyphs[idx].advanceX ? style.font.glyphs[idx].advanceX : style.font.recs[idx].width;
            pos.x += style.cSpacing + scaleFactor*w;
        }
    }
    return res;
}

Gui_Label gui_newLabel(Rectangle bounds, char *text, Gui_El_Style defaultStyle, Gui_El_Style hovered)
{
    char *arrList = NULL;
    i32 text_len  = text == NULL ? 0 : TextLength(text);
    stbds_arrsetlen(arrList, text_len + 1);
    if (text_len > 0) memcpy(arrList, text, text_len + 1);
    arrList[text_len] = 0;

    return (Gui_Label) {
        // .x            = x - defaultStyle.pad,
        // .y            = y - defaultStyle.pad,
        // .w            = ((i32) size.x) + 2*defaultStyle.pad,
        // .h            = defaultStyle.font_size + 2*defaultStyle.pad,
        .bounds       = bounds,
        .text         = arrList,
        .defaultStyle = defaultStyle,
        .hovered      = hovered,
    };
}

void gui_rmCharLabel(Gui_Label *self, u32 idx)
{
    if (idx >= stbds_arrlen(self->text)) return;
    stbds_arrdel(self->text, idx);
}

void gui_insertCharLabel(Gui_Label *self, i32 idx, char c)
{
    stbds_arrins(self->text, idx, c);
}

void gui_insertSliceLabel(Gui_Label *self, i32 idx, const char *slice, i32 slice_size)
{
    stbds_arrinsn(self->text, idx, slice_size);
    for (i32 i = 0; i < slice_size; i++) {
        self->text[idx + i] = slice[i];
    }
}

Gui_El_State gui_getState(i32 x, i32 y, i32 w, i32 h)
{
    Vector2 mouse = GetMousePosition();
    bool hovered   = gui_isPointInRec((i32) mouse.x, (i32) mouse.y, x, y, w, h);
    if (hovered > 0) return (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) ? EL_STATE_PRESSED : EL_STATE_HOVERED;
    else return EL_STATE_INACTIVE;
}

inline void gui_resizeLabel(Gui_Label *self, Gui_El_State state)
{
    gui_resizeLabelEx(self, state, self->text);
}

Gui_Drawable_Text gui_resizeLabelEx(Gui_Label *self, Gui_El_State state, char *text)
{
    Gui_El_Style style = (state >= EL_STATE_HOVERED) ? self->hovered : self->defaultStyle;
    Gui_Drawable_Text drawable = {0};
    Vector2 size  = gui_measureText(text, self->bounds, style, &drawable);
    self->bounds.width  = size.x + 2*style.pad;
    self->bounds.height = size.y + 2*style.pad;
    return drawable;
}

Vector2 gui_measureLabelText(Gui_Label self, Gui_El_State state)
{
    Gui_El_Style style = (state >= EL_STATE_HOVERED) ? self.hovered : self.defaultStyle;
    return MeasureTextEx(style.font, self.text, style.font_size, style.cSpacing);
}

Gui_El_State gui_drawLabel(Gui_Label self)
{
    Gui_El_State state = gui_getState(self.bounds.x, self.bounds.y, self.bounds.width, self.bounds.height);
    if (state == EL_STATE_HIDDEN) return state;
    bool hovered = state >= EL_STATE_HOVERED;
    Gui_El_Style style   = (hovered) ? self.hovered : self.defaultStyle;

    Gui_Drawable_Text prepText = gui_prepTextForDrawing(self.text, self.bounds, style);
    gui_drawPreparedText(prepText, style);
    if (hovered) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    return state;
}

Gui_Input_Box gui_newInputBox(char *placeholder, bool resize, bool multiline, bool selected, Gui_Label label)
{
    u16 rows = 1;
    u32 i = 0;
    while (i < stbds_arrlen(label.text)) {
        if (label.text[i] == '\n') rows += 1;
        i += 1;
    }

    return (Gui_Input_Box) {
        .placeholder = placeholder,
        .label       = label,
        .cur         = (stbds_arrlen(label.text) == 0) ? 0 : stbds_arrlen(label.text) - 1,
        .anim_idx    = 0,
        .rows        = rows,
        .resize      = resize,
        .multiline   = multiline,
        .selected    = selected,
    };
}

bool gui_isInputBoxHovered(Gui_Input_Box self)
{
    Vector2 mouse = GetMousePosition();
    return gui_isPointInRec((int)mouse.x, (int)mouse.y, self.label.bounds.x, self.label.bounds.y, self.label.bounds.width, self.label.bounds.height);
}

inline Gui_El_State gui_getInputBoxState(Gui_Input_Box *self)
{
    return gui_getInputBoxStateHelper(self, gui_isInputBoxHovered(*self));
}

Gui_El_State gui_getInputBoxStateHelper(Gui_Input_Box *self, bool hovered)
{
    bool EL_STATE_PRESSED = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    if (!hovered && EL_STATE_PRESSED) {
        self->selected = false;
        return EL_STATE_INACTIVE;
    } else if (EL_STATE_PRESSED) {
        self->selected = true;
        return EL_STATE_PRESSED;
    } else if (self->selected) {
        return EL_STATE_FOCUSED;
    } else if (hovered) {
        return EL_STATE_HOVERED;
    } else {
        return EL_STATE_INACTIVE;
    }
}

void gui_resetInputBoxAnim(Gui_Input_Box *self)
{
    self->anim_idx = -Input_Box_anim_len;
}

Gui_Update_Res gui_handleKeysInputBox(Gui_Input_Box *self)
{
    Gui_Update_Res res = {0}; // @TODO: Default values

    if (IsKeyPressed(KEY_ENTER)) {
        if (self->multiline || (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT))) {
            gui_insertCharLabel(&self->label, self->cur, '\n');
            self->cur += 1;
            res.updated = true;
        } else {
            res.enter = true;
        }
    } else if (IsKeyPressed(KEY_TAB)) {
        res.tab = true;
    } else if (IsKeyPressed(KEY_RIGHT)) {
        if (self->cur < stbds_arrlen(self->label.text) - 1) self->cur += 1;
    } else if (IsKeyPressed(KEY_LEFT)) {
        if (self->cur > 0) self->cur -= 1;
    } else if (IsKeyPressed(KEY_UP)) {
        u32 prev_row = 0;
        u32 curr_row = 0;
        u32 i = 0;
        while (i < self->cur) {
            if (self->label.text[i] == '\n') {
                prev_row = curr_row;
                curr_row = i + 1;
            }
            i += 1;
        }
        if (curr_row == 0) {
            self->cur = 0;
        } else {
            u32 d = self->cur - curr_row;
            self->cur = (curr_row - prev_row > d) ? prev_row + d : curr_row - 1;
        }
    } else if (IsKeyPressed(KEY_DOWN)) {
        // @TODO
    } else if (IsKeyPressed(KEY_BACKSPACE)) {
        if (self->cur > 0) {
            gui_rmCharLabel(&self->label, self->cur - 1);
            self->cur -= 1;
            res.updated = true;
        }
    } else if (IsKeyPressed(KEY_DELETE)) {
        if (self->cur < stbds_arrlen(self->label.text) - 1) {
            gui_rmCharLabel(&self->label, self->cur);
            res.updated = true;
        }
    } else {
        i32 codepoint = GetCharPressed();
        if (codepoint > 0) {
            res.updated = true;
            if (codepoint <= 0xff) {
                gui_insertCharLabel(&self->label, self->cur, (u8) codepoint);
                self->cur += 1;
            } else if (codepoint <= 0xffff) {
                char slice[] = {(u8)(codepoint & 0xff00), (u8)(codepoint & 0xff)};
                gui_insertSliceLabel(&self->label, self->cur, slice, 2);
                self->cur += 2;
            } else if (codepoint <= 0xffffff) {
                char slice[] = {(u8)(codepoint & 0xff0000), (u8)(codepoint & 0xff00), (u8)(codepoint & 0xff)};
                gui_insertSliceLabel(&self->label, self->cur, slice, 3);
                self->cur += 3;
            } else {
                char slice[] = {(u8)(codepoint & 0xff000000), (u8)(codepoint & 0xff0000), (u8)(codepoint & 0xff00), (u8)(codepoint & 0xff)};
                gui_insertSliceLabel(&self->label, self->cur, slice, 4);
                self->cur += 4;
            }
        }
    }

    return res;
}

// Draws the Gui_Input_Box, but also does more, like handling any user input if the box is active
Gui_Update_Res gui_drawInputBox(Gui_Input_Box *self)
{
    Gui_Update_Res res = {0}; // @TODO: Default values
    bool hovered  = gui_isInputBoxHovered(*self);
    Gui_El_State state = gui_getInputBoxStateHelper(self, hovered);
    if (state == EL_STATE_HIDDEN) return res;
    Gui_El_Style style = (hovered || gui_stateIsActive(state)) ? self->label.hovered : self->label.defaultStyle;

    char *text = self->label.text;
    if (!text || !text[0]) text = self->placeholder;

    Gui_Drawable_Text prepText = {0};
    if (self->selected) {
        res = gui_handleKeysInputBox(self);
        if (res.updated || self->resize) prepText = gui_resizeLabelEx(&self->label, state, text);
    }

    if (!prepText.lineXs) prepText = gui_prepTextForDrawing(text, self->label.bounds, style);
    Vector2 *coords = gui_drawSizedEx(prepText, self->label.bounds, style);

    // If mouse was clicked on the label, the cursor should be updated correspondingly
    if (state == EL_STATE_PRESSED) {
        Vector2 mouse  = GetMousePosition();
        u32 newCur     = 0;
        float distance = FLT_MAX;
        u16 rows       = 1;
        u32 i          = 0;
        while (i < prepText.text_len) {
            Vector2 c = coords[i];
            if (i + 1 == prepText.text_len && c.x < mouse.x) {
                newCur = prepText.text_len;
                break;
            } else if (mouse.x < c.x) {
                float d = mouse.y - c.y;
                if (d < 0) {
                    newCur = i;
                    break;
                } else if (d < distance) {
                    newCur   = i;
                    distance = d;
                    if (rows == self->rows) break;
                }
            }
            i += 1;
        }
        self->cur = newCur;
        self->anim_idx = -Input_Box_anim_wait;
    }

    // Display cursor
    {
        float anim_len_half = (float)(Input_Box_anim_len)/2.0f;
        i32 ai = (self->anim_idx < 0) ? 0 : self->anim_idx;
            ai = anim_len_half - ai;
        if (ai < 0) ai = -ai;
        float scale  = style.font_size;
        float height = scale * ((float)(ai))/anim_len_half;

        float x, y;
        if (prepText.text_len == 0) {
            x = (float)(self->label.bounds.x + style.pad);
            y = (float)(self->label.bounds.y + style.pad);
        } else if (self->cur < prepText.text_len) {
            x = coords[self->cur].x;
            y = coords[self->cur].y;
        } else {
            i32 cp_size    = 0;
            i32 cp         = GetCodepointNext(&self->label.text[prepText.text_len-1], &cp_size);
            float advanceX = (float)(GetGlyphInfo(style.font, cp).advanceX);
            x = coords[prepText.text_len-1].x + advanceX;
            y = coords[prepText.text_len-1].y;
        }

        x -= ((float) Input_Box_cur_width)/2.0f;
        y += (scale - height)/2.0f;
        DrawRectangle((i32) x, (i32) y, Input_Box_cur_width, (i32) height, Input_Box_cur_color);

        self->anim_idx += 1;
        if (self->anim_idx >= Input_Box_anim_len) self->anim_idx = 0;
    }

    GUI_FREE(coords);
    if (hovered) SetMouseCursor(MOUSE_CURSOR_IBEAM);
    res.state = state;
    return res;
}

#endif // _GUI_IMPL_GUARD_
#endif // GUI_IMPLEMENTATION