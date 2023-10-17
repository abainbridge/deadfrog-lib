// Own header.
#include "df_gui.h"

// Deadfrog lib headers.
#include "fonts/df_prop.h"
#include "df_bitmap.h"
#include "df_time.h"
#include "df_window.h"

// Standard headers.
#include <math.h>
#include <string.h>


DfColour g_backgroundColour = { 0xff303030 };
DfColour g_frameColour = { 0xff454545 };
DfColour g_buttonShadowColour = { 0xff2e2e2e };
DfColour g_buttonColour = { 0xff5a5a5a };
DfColour g_buttonHighlightColour = { 0xff6f6f6f };
DfColour g_normalTextColour = Colour(210, 210, 210, 255);
DfColour g_selectionColour = Colour(18, 71, 235);

double g_drawScale = 1.0;
int g_dragStartX;
int g_dragStartY;


// ****************************************************************************
// Misc functions
// ****************************************************************************

static int PointInRect(int px, int py, int rx, int ry, int rw, int rh) {
    return px >= rx && 
           px < (rx + rw) &&
           py >= ry && 
           py < (ry + rh);
}


int DfMouseInRect(DfWindow *win, int x, int y, int w, int h) {
    return PointInRect(win->input.mouseX, win->input.mouseY, x, y, w, h);
}


void DfGuiDoFrame(DfWindow *win) {
    if (win->input.lmbClicked) {
        g_dragStartX = win->input.mouseX;
        g_dragStartY = win->input.mouseY;
    }

    int scaleChanged = 0;
    if (win->input.keys[KEY_CONTROL]) {
        if (win->input.keyDowns[KEY_EQUALS] || win->input.keyDowns[KEY_PLUS_PAD]) {
            g_drawScale *= 1.1;
            scaleChanged = 1;
        }
        else if (win->input.keyDowns[KEY_MINUS] || win->input.keyDowns[KEY_MINUS_PAD]) {
            g_drawScale /= 1.1;
            scaleChanged = 1;
        }

    }

    if (!g_defaultFont || scaleChanged) {
        g_drawScale = ClampDouble(g_drawScale, 0.7, 3.0);
        double desiredHeight = g_drawScale * 13.0;
        int bestFontIdx = 0;
        char bestDeltaSoFar = fabs(df_prop.pixelHeights[0] - desiredHeight);
        for (int i = 1; i < df_prop.numSizes; i++) {
            double delta = fabs(desiredHeight - df_prop.pixelHeights[i]);
            if (delta < bestDeltaSoFar) {
                bestDeltaSoFar = delta;
                bestFontIdx = i;
            }
        }

        g_defaultFont = LoadFontFromMemory(df_prop.dataBlobs[bestFontIdx],
            df_prop.dataBlobsSizes[bestFontIdx]);
    }
}


void DfDrawSunkenBox(DfBitmap *bmp, int x, int y, int w, int h) {
    // The specified w and h is the external size of the box.
    //
    //        <-------- w -------->
    //     ^  1 1 1 1 1 1 1 1 1 1 1  ^
    //     |  1 1 1 1 1 1 1 1 1 1 1  | thickness
    //     |  1 1 1 1 1 1 1 1 1 1 1  v
    //     |  3 3 3           4 4 4
    //   h |  3 3 3           4 4 4
    //     |  3 3 3           4 4 4
    //     |  2 2 2 2 2 2 2 2 2 2 2
    //     |  2 2 2 2 2 2 2 2 2 2 2
    //     v  2 2 2 2 2 2 2 2 2 2 2
    //       <--->
    //     thickness

    int thickness = RoundToInt(g_drawScale * 1.5);
    DfColour dark = g_buttonShadowColour;
    RectFill(bmp, x, y, w, thickness, dark); // '1' pixels
    RectFill(bmp, x, y + h - thickness, w, thickness, dark); // '2' pixels
    RectFill(bmp, x, y + thickness, thickness, h - 2 * thickness, dark);
    RectFill(bmp, x + w - thickness, y + thickness, thickness, h - 2 * thickness, dark);

    x += thickness;
    y += thickness;
    w -= thickness * 2;
    h -= thickness * 2;
    RectFill(bmp, x, y, w, h, g_backgroundColour);
}


// ****************************************************************************
// V Scrollbar
// ****************************************************************************

int DfVScrollbarDo(DfWindow *win, DfVScrollbar *vs, int x, int y, int w, int h, int hasFocus) {
    if (hasFocus)
        vs->currentVal -= win->input.mouseVelZ * 0.3;

    int mouseInBounds = DfMouseInRect(win, x, y, w, h);
    if (win->input.lmbClicked && mouseInBounds) {
        vs->dragging = true;
        vs->currentValAtDragStart = vs->currentVal;
    }
    else if (!win->input.lmb) {
        vs->dragging = false;
    }

    if (vs->dragging) {
        int dragY = win->input.mouseY - g_dragStartY;
        if (dragY != 0)
            dragY = dragY;
        double scale = vs->maximum / (double)h;
        vs->currentVal = vs->currentValAtDragStart + dragY * scale;
    }

    if (vs->currentVal < 0)
        vs->currentVal = 0;
    else if (vs->currentVal + vs->coveredRange > vs->maximum)
        vs->currentVal = vs->maximum - vs->coveredRange;

    RectFill(win->bmp, x, y, w, h, g_frameColour);

    int handleGap = g_drawScale;
    y += handleGap;
    h -= handleGap * 2;
    float inv_max = (float)h / (float)vs->maximum;
    float currentValAsFraction = vs->currentVal * inv_max;
    float covered_range_as_fraction = vs->coveredRange * inv_max;
    int handle_x = x + handleGap;
    int handle_y = y + currentValAsFraction;
    int handle_w = w - 2 * handleGap;
    int handle_h = covered_range_as_fraction;

    DfColour handleColour = g_buttonColour;
    if (mouseInBounds || vs->dragging)
        handleColour = g_buttonHighlightColour;
    RectFill(win->bmp, handle_x, handle_y, handle_w, handle_h, handleColour);
    return 0;
}


// ****************************************************************************
// Edit box
// ****************************************************************************

// Returns 1 if contents changed.
int DfEditBoxDo(DfWindow *win, DfEditBox *eb, int x, int y, int w, int h) {
    DfDrawSunkenBox(win->bmp, x, y, w, h);
    x += 2 * g_drawScale;
    y += 4 * g_drawScale;
    w -= 4 * g_drawScale;
    h -= 8 * g_drawScale;
    SetClipRect(win->bmp, x, y, w, h);

    double now = GetRealTime();
    if (now > eb->nextCursorToggleTime) {
        eb->cursorOn = !eb->cursorOn;
        eb->nextCursorToggleTime = now + 0.5;
    }

    if (win->input.keyDowns[KEY_LEFT]) {
        eb->cursorIdx = IntMax(0, eb->cursorIdx - 1);
        eb->nextCursorToggleTime = now;
    }
    else if (win->input.keyDowns[KEY_RIGHT]) {
        eb->cursorIdx = IntMin(strlen(eb->text), eb->cursorIdx + 1);
        eb->nextCursorToggleTime = now;
    }
    else if (win->input.keyDowns[KEY_HOME]) {
        eb->cursorIdx = 0;
        eb->nextCursorToggleTime = now;
    }
    else if (win->input.keyDowns[KEY_END]) {
        eb->cursorIdx = strlen(eb->text);
        eb->nextCursorToggleTime = now;
    }

    int contentsChanged = 0;
    for (int i = 0; i < win->input.numKeysTyped; i++) {
        char c = win->input.keysTyped[i];
        if (c == 8) { // Backspace
            if (eb->cursorIdx > 0) {
                char *move_src = eb->text + eb->cursorIdx;
                unsigned move_size = strlen(move_src);
                memmove(move_src - 1, move_src, move_size);
                move_src[move_size - 1] = '\0';
                eb->cursorIdx--;
            }
        }
        else if (c == 127) { // Delete
            char *moveSrc = eb->text + eb->cursorIdx + 1;
            unsigned move_size = strlen(moveSrc);
            memmove(moveSrc - 1, moveSrc, move_size);
            moveSrc[move_size - 1] = '\0';
        }
        else {
            char *moveSrc = eb->text + eb->cursorIdx;
            unsigned move_size = strlen(moveSrc);
            memmove(moveSrc + 1, moveSrc, move_size);
            eb->text[eb->cursorIdx] = c;
            eb->text[sizeof(eb->text) - 1] = '\0';
            eb->cursorIdx = IntMin(eb->cursorIdx + 1, sizeof(eb->text) - 1);
        }

        contentsChanged = 1;
        eb->nextCursorToggleTime = now;
    }

    DrawTextSimple(g_defaultFont, g_normalTextColour, win->bmp, x, y, eb->text);

    if (eb->cursorOn) {
        int cursorX = GetTextWidthNumChars(g_defaultFont, eb->text, eb->cursorIdx) + x;
        RectFill(win->bmp, cursorX, y, 2 * g_drawScale, g_defaultFont->charHeight, g_normalTextColour);
    }

    ClearClipRect(win->bmp);

    return contentsChanged;
}


// ****************************************************************************
// List View
// ****************************************************************************

int DfListViewDo(DfWindow *win, DfListView *lv, int x, int y, int w, int h) {
    DfDrawSunkenBox(win->bmp, x, y, w, h);
    x += 2 * g_drawScale;
    y += 2 * g_drawScale;
    w -= 4 * g_drawScale;
    h -= 4 * g_drawScale;
    SetClipRect(win->bmp, x, y, w, h);

    const int numRows = RoundToInt(h / (double)g_defaultFont->charHeight - 0.9);

    if (win->input.keyDowns[KEY_DOWN]) {
        lv->selectedItem++;
        lv->firstDisplayItem = IntMax(lv->selectedItem - numRows, lv->firstDisplayItem);
    }
    else if (win->input.keyDowns[KEY_UP]) {
        lv->selectedItem--;
        lv->firstDisplayItem = IntMin(lv->selectedItem, lv->firstDisplayItem);
    }
    else if (win->input.keyDowns[KEY_PGDN]) {
        lv->selectedItem += numRows;
        lv->firstDisplayItem += numRows;
    }
    else if (win->input.keyDowns[KEY_PGUP]) {
        lv->selectedItem -= numRows;
        lv->firstDisplayItem -= numRows;
    }

    if (win->input.lmbClicked && DfMouseInRect(win, x, y, w, h)) {
        int row = (win->input.mouseY - y) / g_defaultFont->charHeight;
        lv->selectedItem = row + lv->firstDisplayItem;
    }

    if (lv->selectedItem >= lv->numItems || lv->numItems <= 0)
        lv->selectedItem = lv->numItems - 1;
    else if (lv->selectedItem < 0)
        lv->selectedItem = 0;

    lv->firstDisplayItem -= win->input.mouseVelZ / 36;
    lv->firstDisplayItem = ClampInt(lv->firstDisplayItem, 0, lv->numItems - numRows);
    if (lv->numItems <= numRows) lv->firstDisplayItem = 0;

    int last_y = y + h;
    for (int i = lv->firstDisplayItem; i < lv->numItems; i++) {
        if (y > last_y) break;

        if (i == lv->selectedItem) {
            RectFill(win->bmp, x, y, w, g_defaultFont->charHeight, g_selectionColour);
        }

        DrawTextSimple(g_defaultFont, g_normalTextColour, win->bmp,
            x + 2 * g_drawScale, y, lv->items[i]);
        y += g_defaultFont->charHeight;
    }

    ClearClipRect(win->bmp);

    return -1;
}


// ****************************************************************************
// Text View
// ****************************************************************************

void DfTextViewEmpty(DfTextView *tv) {
    tv->text[0] = '\0';
    tv->selectionStartX = tv->selectionEndX;
    tv->selectionStartY = tv->selectionEndY;
}


void DfTextViewAddText(DfTextView *tv, char const *text) {
    int currentLen = strlen(tv->text);
    int additionalLen = strlen(text);
    int space = TEXT_VIEW_MAX_CHARS - currentLen - 1;
    int amtToCopy = IntMin(space, additionalLen);
    memcpy(tv->text + currentLen, text, amtToCopy);
    tv->text[currentLen + amtToCopy] = '\0';
}


static char const *FindSpace(char const *c) {
    while (*c != ' ' && *c != '\n' && *c != '\0')
        c++;
    return c;
}


static int TextViewWrapText(DfTextView *tv, int w) {
    int spacePixels = GetTextWidth(g_defaultFont, " ");
    char const *wordIn = tv->text;
    char *wordOut = tv->wrappedText;
    int currentX = 0;
    int y = 0;

    // For each word...
    while (1) {
        char const *space = FindSpace(wordIn);
        unsigned wordLen = space - wordIn;

        int numPixels = GetTextWidthNumChars(g_defaultFont, wordIn, wordLen);
        if (currentX + numPixels >= w) {
            currentX = 0;
            y += g_defaultFont->charHeight;
            *wordOut = '\n';
            wordOut++;
        }

        if (wordOut + wordLen >= tv->wrappedText + sizeof(tv->wrappedText)) {
            *wordOut = '\0';
            break;
        }

        memcpy(wordOut, wordIn, wordLen);
        wordOut += wordLen;

        currentX += numPixels + spacePixels;
        wordIn += wordLen;
        *wordOut = *space;
        wordOut++;

        if (*space == '\n') {
            currentX = 0;
            y += g_defaultFont->charHeight;
        }

        if (*wordIn == '\0') {
            *wordOut = '\0';
            break;
        }

        wordIn++;
    }

    return y;
}


static int GetSelectionCoords(DfTextView *tv, int *sx, int *sy, int *ex, int *ey) {
    int xIsEqual = (tv->selectionStartX == tv->selectionEndX);
    int yIsEqual = (tv->selectionStartY == tv->selectionEndY);
    if (xIsEqual && yIsEqual) {
        *sx = *sy = *ex = *ey = -1;
        return 0;
    }

    int swapNeeded = (tv->selectionStartY > tv->selectionEndY);
    if (yIsEqual && tv->selectionStartX > tv->selectionEndX)
        swapNeeded = 1;

    if (swapNeeded) {
        *sx = tv->selectionEndX;
        *sy = tv->selectionEndY;
        *ex = tv->selectionStartX;
        *ey = tv->selectionStartY;
    }
    else {
        *sx = tv->selectionStartX;
        *sy = tv->selectionStartY;
        *ex = tv->selectionEndX;
        *ey = tv->selectionEndY;
    }

    return 1;
}


static int GetStringIdxFromPixelOffset(char const *s, int targetPixelOffset) {
    int pixelOffset = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        pixelOffset += GetTextWidthNumChars(g_defaultFont, s + i, 1);
        if (pixelOffset > targetPixelOffset) {
            return i;
        }
    }

    return 0;
}


void DfTextViewDo(DfWindow *win, DfTextView *tv, int x, int y, int w, int h) {
    int mouseInRect = DfMouseInRect(win, x, y, w, h);
    DfDrawSunkenBox(win->bmp, x, y, w, h);

    int borderWidth = 2 * g_drawScale;
    x += 2 * borderWidth;
    y += 1 * borderWidth;
    w -= 4 * borderWidth;
    h -= 2 * borderWidth;
    SetClipRect(win->bmp, x, y, w, h);

    int scrollbar_w = 10 * g_drawScale;
    int scrollbar_x = x + w - scrollbar_w;
    int scrollbar_y = y + 1 * borderWidth;
    int scrollbar_h = h - 2 * borderWidth;

    int textRight = scrollbar_x;
    int space_pixels = GetTextWidth(g_defaultFont, " ");
    char const *line = tv->wrappedText;

    int total_pixel_height = TextViewWrapText(tv, w - scrollbar_w);

    tv->v_scrollbar.coveredRange = scrollbar_h;
    tv->v_scrollbar.maximum = total_pixel_height;
    if (tv->v_scrollbar.maximum < scrollbar_h)
        tv->v_scrollbar.maximum = scrollbar_h;
    DfVScrollbarDo(win, &tv->v_scrollbar, scrollbar_x, scrollbar_y, scrollbar_w, scrollbar_h, mouseInRect);

    int sel_start_x, sel_start_y, sel_end_x, sel_end_y;
    GetSelectionCoords(tv, &sel_start_x, &sel_start_y, &sel_end_x, &sel_end_y);

    y -= tv->v_scrollbar.currentVal;
    for (int line_num = 0; ; line_num++) {
        // Update selection block if mouse click on current line.
        if (mouseInRect) {
            if (win->input.mouseY >= y && win->input.mouseY < (y + g_defaultFont->charHeight)) {
                if (win->input.lmbClicked) {
                    int pixel_offset = win->input.mouseX - x;
                    tv->selectionStartX = GetStringIdxFromPixelOffset(line, pixel_offset);
                    tv->selectionStartY = line_num;
                }

                if (win->input.lmb) {
                    int pixel_offset = win->input.mouseX - x;
                    tv->selectionEndX = GetStringIdxFromPixelOffset(line, pixel_offset);
                    tv->selectionEndY = line_num;
                }
            }
        }

        char const *end_of_line = strchr(line, '\n');
        if (!end_of_line) break;
        int line_len = end_of_line - line - 1;

        // Draw selection block
        if (line_num >= sel_start_y && line_num <= sel_end_y) {
            int start_idx = 0;
            int end_idx = line_len;
            if (line_num == sel_start_y)
                start_idx = sel_start_x;
            if (line_num == sel_end_y && (sel_end_x + 1) < line_len)
                end_idx = sel_end_x + 1;

            int sel_x = x + GetTextWidthNumChars(g_defaultFont, line, start_idx);
            int sel_w = GetTextWidthNumChars(g_defaultFont, line + start_idx, end_idx - start_idx);
            RectFill(win->bmp, sel_x, y, sel_w, g_defaultFont->charHeight, g_selectionColour);
        }

        DrawTextSimpleLen(g_defaultFont, g_normalTextColour, win->bmp,
            x, y, line, line_len);
        line = end_of_line + 1;
        y += g_defaultFont->charHeight;
    }

    ClearClipRect(win->bmp);
}


char const *DfTextViewGetSelectedText(DfTextView *tv, int *num_chars) {
    int selStartX, selStartY, selEndX, selEndY;
    if (GetSelectionCoords(tv, &selStartX, &selStartY, &selEndX, &selEndY) == 0)
        return NULL;

    // Find the start of the selected text.
    char const *line = tv->wrappedText;
    int lineNum = 0;
    while (lineNum < selStartY) {
        char const *endOfLine = strchr(line, '\n');
        DebugAssert(endOfLine);
        line = endOfLine + 1;
        lineNum++;
    }

    char const *startOfBlock = line + selStartX;

    // Find the end of the selected text.
    while (lineNum < selEndY) {
        char const *endOfLine = strchr(line, '\n');
        DebugAssert(endOfLine);
        line = endOfLine + 1;
        lineNum++;
    }

    *num_chars = line - startOfBlock + selEndX + 1;

    return startOfBlock;
}



// ****************************************************************************
// Button
// ****************************************************************************

int DfButtonDo(DfWindow *win, DfButton *b, int x, int y, int w, int h) {
    int MouseInRect = DfMouseInRect(win, x, y, w, h);
    DfColour handleColour = g_buttonColour;
    if (MouseInRect)
        handleColour = g_buttonHighlightColour;
    RectFill(win->bmp, x, y, w, h, handleColour);
    DrawTextCentre(g_defaultFont, g_normalTextColour, win->bmp, 
        x + w / 2, y + h / 5, b->label);
    return MouseInRect && win->input.lmbClicked;
}
