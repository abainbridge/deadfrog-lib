// Own header.
#include "df_gui.h"

// Deadfrog lib headers.
#include "fonts/df_prop.h"
#include "df_bitmap.h"
#include "df_clipboard.h"
#include "df_time.h"
#include "df_window.h"

// Standard headers.
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>


DfColour g_backgroundColour = { 0xff303030 };
DfColour g_frameColour = { 0xff454545 };
DfColour g_buttonShadowColour = { 0xff2e2e2e };
DfColour g_buttonColour = { 0xff5a5a5a };
DfColour g_buttonHighlightColour = { 0xff6f6f6f };
DfColour g_normalTextColour = Colour(210, 210, 210, 255); // TODO change to g_textColour.
DfColour g_selectionColour = Colour(48, 81, 250);

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


static int ChangeDrawScaleIfDpiChanged() {
	static int lastDpi = 96;
	static float prevDrawScale = 1.0f;

	int dpi = GetMonitorDpi(g_window);

	bool scaleChanged = (dpi != lastDpi);
	if (scaleChanged) {
		g_drawScale = prevDrawScale / (float)lastDpi * (float)dpi;
		int width = g_window->bmp->width / prevDrawScale * g_drawScale;
		int height = g_window->bmp->width / prevDrawScale * g_drawScale;
        lastDpi = dpi;
        SetWindowRect(g_window, g_window->left, g_window->top, width, height);
	}

	prevDrawScale = g_drawScale;

	return scaleChanged;
}


void DfGuiDoFrame(DfWindow *win) {
    if (win->input.lmbClicked) {
        g_dragStartX = win->input.mouseX;
        g_dragStartY = win->input.mouseY;
    }

    int scaleChanged = ChangeDrawScaleIfDpiChanged();

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

#define CURSOR_TOGGLE_PERIOD 0.5

static void EditBoxGetSelectionIndices(DfEditBox *eb, int *c1, int *c2) {
    *c1 = eb->cursorIdx;
    *c2 = eb->selectionIdx;
    if (*c1 > *c2) {
        *c2 = eb->cursorIdx;
        *c1 = eb->selectionIdx;
    }
}


static void EditBoxForceCusorOn(DfEditBox *eb) {
    eb->cursorOn = 1;
    double now = GetRealTime();
    eb->nextCursorToggleTime = now + CURSOR_TOGGLE_PERIOD;
}


static void EditBoxDeleteChars(DfEditBox *eb, int startIdx, int numChars) {
    char *c = eb->text + startIdx;
    int cLen = strlen(c);
    numChars = IntMin(numChars, cLen);
    memmove(c, c + numChars, cLen - numChars);
    c[cLen - numChars] = '\0';
}


static void EditBoxInsertChar(DfEditBox *eb, char c) {
    if (c < 32)
        return;
    unsigned len = strlen(eb->text);
    if (len >= (sizeof(eb->text) - 1))
        return;
    char *moveSrc = eb->text + eb->cursorIdx;
    unsigned moveSize = len - eb->cursorIdx;
    memmove(moveSrc + 1, moveSrc, moveSize);
    eb->text[eb->cursorIdx] = c;
    moveSrc[moveSize + 1] = '\0';
    eb->cursorIdx = IntMin(eb->cursorIdx + 1, sizeof(eb->text) - 1);
}


static void EditBoxMoveCursorWordLeft(DfEditBox *eb) {
    int startIsAlnum = isalnum(eb->text[eb->cursorIdx - 1]);
    while (eb->cursorIdx > 0 && isalnum(eb->text[eb->cursorIdx - 1]) == startIsAlnum) {
        eb->cursorIdx--;
    }
}


static void EditBoxMoveCursorWordRight(DfEditBox *eb) {
    int startIsAlnum = isalnum(eb->text[eb->cursorIdx]);
    int len = strlen(eb->text);
    while (eb->cursorIdx < len && isalnum(eb->text[eb->cursorIdx]) == startIsAlnum) {
        eb->cursorIdx++;
    }
}


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
        eb->nextCursorToggleTime = now + CURSOR_TOGGLE_PERIOD;
    }

    // Process keyboard shortcuts.
    {
        int clearSelection = 0;

        if (win->input.keyDowns[KEY_LEFT]) {
            if (win->input.keys[KEY_CONTROL])
                EditBoxMoveCursorWordLeft(eb);
            else
                eb->cursorIdx = IntMax(0, eb->cursorIdx - 1);
            clearSelection = !win->input.keys[KEY_SHIFT];
            EditBoxForceCusorOn(eb);
        }
        else if (win->input.keyDowns[KEY_RIGHT]) {
            if (win->input.keys[KEY_CONTROL])
                EditBoxMoveCursorWordRight(eb);
            else
                eb->cursorIdx = IntMin(strlen(eb->text), eb->cursorIdx + 1);
            clearSelection = !win->input.keys[KEY_SHIFT];
            EditBoxForceCusorOn(eb);
        }
        else if (win->input.keyDowns[KEY_HOME]) {
            eb->cursorIdx = 0;
            clearSelection = !win->input.keys[KEY_SHIFT];
            EditBoxForceCusorOn(eb);
        }
        else if (win->input.keyDowns[KEY_END]) {
            eb->cursorIdx = strlen(eb->text);
            clearSelection = !win->input.keys[KEY_SHIFT];
            EditBoxForceCusorOn(eb);
        }
        else if (win->input.keys[KEY_CONTROL]) {
            if (win->input.keyDowns[KEY_A]) { // Select all
                eb->selectionIdx = 0;
                eb->cursorIdx = strlen(eb->text);
                EditBoxForceCusorOn(eb);
            }
            else if (win->input.keyDowns[KEY_BACKSPACE]) { // Delete word left
                eb->selectionIdx = eb->cursorIdx;
                EditBoxMoveCursorWordLeft(eb);
                EditBoxDeleteChars(eb, eb->cursorIdx, eb->selectionIdx - eb->cursorIdx + 1);
                clearSelection = 1;
                EditBoxForceCusorOn(eb);
            }
            else if (win->input.keyDowns[KEY_DEL]) { // Delete word right
                eb->selectionIdx = eb->cursorIdx;
                EditBoxMoveCursorWordRight(eb);
                EditBoxDeleteChars(eb, eb->selectionIdx, eb->cursorIdx - eb->selectionIdx + 1);
                eb->cursorIdx = eb->selectionIdx;
                EditBoxForceCusorOn(eb);
            }
            else if (win->input.keyDowns[KEY_C]) { // Copy to clipboard
                int c1, c2;
                EditBoxGetSelectionIndices(eb, &c1, &c2);
                ClipboardSetData(eb->text + c1, c2 - c1);
            }
            else if (win->input.keyDowns[KEY_X]) { // Cut to clipboard
                int c1, c2;
                EditBoxGetSelectionIndices(eb, &c1, &c2);
                ClipboardSetData(eb->text + c1, c2 - c1);
                EditBoxDeleteChars(eb, c1, c2 - c1);
                eb->cursorIdx = eb->selectionIdx = c1;
                clearSelection = 1;
                EditBoxForceCusorOn(eb);
            }
            else if (win->input.keyDowns[KEY_V]) { // Paste from clipboard
                int c1, c2;
                EditBoxGetSelectionIndices(eb, &c1, &c2);
                EditBoxDeleteChars(eb, c1, c2 - c1);
                eb->cursorIdx = eb->selectionIdx = c1;

                int clipBufNumChars;
                char *clipBuf = ClipboardReceiveData(&clipBufNumChars);
                for (int i = 0; i < clipBufNumChars; i++)
                    EditBoxInsertChar(eb, clipBuf[i]);
                ClipboardReleaseReceivedData(clipBuf);

                clearSelection = 1;
                EditBoxForceCusorOn(eb);
            }
        }

        if (clearSelection) {
            eb->selectionIdx = eb->cursorIdx;
        }
    }

    // Process what the user typed.
    int contentsChanged = 0;
    if (win->input.numKeysTyped > 0) {
        int selectionDeleted = 0;
        if (eb->selectionIdx != eb->cursorIdx) {
            // Delete the selected text and clear the selection.
            int c1, c2;
            EditBoxGetSelectionIndices(eb, &c1, &c2);
            EditBoxDeleteChars(eb, c1, c2 - c1);
            eb->selectionIdx = eb->cursorIdx = c1;
            selectionDeleted = 1;
        }

        for (int i = 0; i < win->input.numKeysTyped; i++) {
            char c = win->input.keysTyped[i];
            if (c == 8) { // Backspace
                if (eb->cursorIdx > 0 && !selectionDeleted) {
                    EditBoxDeleteChars(eb, eb->cursorIdx - 1, 1);
                    eb->cursorIdx--;
                }
            }
            else if (c == 127) { // Delete
                if (!selectionDeleted && !win->input.keys[KEY_CONTROL]) {
                    EditBoxDeleteChars(eb, eb->cursorIdx, 1);
                }
            }
            else {
                EditBoxInsertChar(eb, c);
            }
        }

        EditBoxForceCusorOn(eb);
        eb->selectionIdx = eb->cursorIdx;
        contentsChanged = 1;
    }

    // Do we need to scroll to keep the cursor visible?
    {
        int cursorX = GetTextWidthNumChars(g_defaultFont, eb->text, eb->cursorIdx);
        if (cursorX > (w * 0.95)) {
            x += w * 0.95 - cursorX;
        }
    }

    // Draw selection rectangle.
	if (eb->selectionIdx != eb->cursorIdx) {
        int c1, c2;
        EditBoxGetSelectionIndices(eb, &c1, &c2);
		int x1 = GetTextWidthNumChars(g_defaultFont, eb->text, c1);
		int x2 = GetTextWidthNumChars(g_defaultFont, eb->text + c1, c2 - c1);
		RectFill(win->bmp, x + x1, y, x2, g_defaultFont->charHeight, g_selectionColour);
	}

    DrawTextSimple(g_defaultFont, g_normalTextColour, win->bmp, x, y, eb->text);

    if (eb->cursorOn) {
        int cursorX = GetTextWidthNumChars(g_defaultFont, eb->text, eb->cursorIdx) + x;
        RectFill(win->bmp, cursorX, y, g_drawScale * 1.4, g_defaultFont->charHeight, g_normalTextColour);
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
    int space = DF_TEXT_VIEW_MAX_CHARS - currentLen - 1;
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


static void TextViewSelectWord(DfTextView *tv, char const *line) {
    int startIsAlnum = !!isalnum(line[tv->selectionStartX]);
    while (tv->selectionStartX > 0 && !!isalnum(line[tv->selectionStartX - 1]) == startIsAlnum)
        tv->selectionStartX--;

    int len = strlen(line);
    while ((tv->selectionEndX + 1) < len && !!isalnum(line[tv->selectionEndX + 1]) == startIsAlnum)
        tv->selectionEndX++;
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

    tv->vScrollbar.coveredRange = scrollbar_h;
    tv->vScrollbar.maximum = total_pixel_height;
    if (tv->vScrollbar.maximum < scrollbar_h)
        tv->vScrollbar.maximum = scrollbar_h;
    DfVScrollbarDo(win, &tv->vScrollbar, scrollbar_x, scrollbar_y, scrollbar_w, scrollbar_h, mouseInRect);

    int sel_start_x, sel_start_y, sel_end_x, sel_end_y;
    GetSelectionCoords(tv, &sel_start_x, &sel_start_y, &sel_end_x, &sel_end_y);

    y -= tv->vScrollbar.currentVal;
    for (int line_num = 0; ; line_num++) {
        // Update selection block if mouse click on current line.
        if (mouseInRect) {
            if (win->input.mouseY >= y && win->input.mouseY < (y + g_defaultFont->charHeight)) {
                if (win->input.lmbDoubleClicked) {
                    TextViewSelectWord(tv, line);
                }
                else if (win->input.lmbClicked) {
                    int pixel_offset = win->input.mouseX - x;
                    tv->selectionStartX = GetStringIdxFromPixelOffset(line, pixel_offset);
                    tv->selectionStartY = line_num;
                    tv->selectionEndX = tv->selectionStartX;
                    tv->selectionEndY = tv->selectionStartY;
                    tv->dragSelecting = 1;
                }
                else if (!win->input.lmb) {
                    tv->dragSelecting = false;
                }
                else if (tv->dragSelecting) {
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
                end_idx = sel_end_x;

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


char const *DfTextViewGetSelectedText(DfTextView *tv, int *numChars) {
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

    *numChars = line - startOfBlock + selEndX;

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



// ****************************************************************************
// Menu Bar and Keyboard Shortcuts
// ****************************************************************************

enum {
    MENU_BAR_LEFT_SPACE = 3,
    MENU_BAR_TOP_SPACE = 3,
    MENU_BAR_BOTTOM_SPACE = 3,
    MENU_ITEM_X_SPACE = 9,
    MENU_ITEM_Y_SPACE = 5
};


struct MenuItem {
    char *m_shortcutDisplayString;
    DfKeyboardShortcut m_shortcut;

    // All the public pointers are owned by the creator, eg the caller of Menu::AddItem().
    char const *m_label;
    int         m_hotKeyIndex;  // Index into m_label, indicating which character is the keyboard hot-key for that item
    int         m_top;

    bool IsSeparator() { return !m_label; }
};


struct Menu {
    char const *m_name;
    MenuItem *m_items[10];
    int m_numItems;
    int m_left;
    int m_top; // Top of menu items, not title.
    int m_titleWidth; // Width of the clickable area. Includes space around text.
    int m_longestShortcutLen;   // Length (in pixels) of the longest shortcut display string on this menu
    int m_highlightedItem;

    Menu(char const *name) {
        m_name = name;
        m_numItems = 0;
        m_highlightedItem = -1;
        m_longestShortcutLen = 0;
    }

    void AddItem(char const *label, DfKeyboardShortcut shortcut) {
        if (m_numItems >= 10) return;

        MenuItem *item = new MenuItem;
        item->m_shortcut = shortcut;
        item->m_label = label;
        item->m_hotKeyIndex = 0;

        if (shortcut.key) {
            char buf[64] = "";
            if (shortcut.ctrl) strcat(buf, "Ctrl+");
            if (shortcut.shift) strcat(buf, "Shift+");
            if (shortcut.alt) strcat(buf, "Alt+");
            strcat(buf, GetKeyName(shortcut.key));
            item->m_shortcutDisplayString = strdup(buf);
        }
        else {
            item->m_shortcutDisplayString = NULL;
        }

        m_items[m_numItems] = item;
        m_numItems++;
    }

    DfGuiAction Advance(DfWindow *win) {
        // If key up/down, set new highlighted item
        if (g_window->input.keyDowns[KEY_DOWN] || g_window->input.keyDowns[KEY_UP]) {
            int direction = 0;
            if (g_window->input.keyDowns[KEY_DOWN]) direction = 1;
            if (g_window->input.keyDowns[KEY_UP]) direction = -1;
            if (direction) {
                do {
                    m_highlightedItem += direction;
                    m_highlightedItem += m_numItems;
                    m_highlightedItem %= m_numItems;
                } while (m_items[m_highlightedItem]->IsSeparator());
            }
        }

        bool const mouseMoved = g_window->input.mouseVelX != 0 || g_window->input.mouseVelY != 0;

        // Update highlighted menu item based on mouse position
        if (mouseMoved)
            m_highlightedItem = GetItemUnderMouse(win);

        // If a hot key has been pressed, execute corresponding menu item
        for (int i = 0; i < win->input.numKeysTyped; i++) {
            MenuItem *item = FindMenuItemByHotkey(win->input.keysTyped[i]);
            if (item) {
                return { NULL, item->m_label, item->m_shortcut };
            }
        }

        // If item clicked on or return pressed, execute highlighted menu item
        if (m_highlightedItem != -1 &&
               (g_window->input.keyDowns[KEY_ENTER] || g_window->input.lmbUnClicked)) {
            MenuItem *item = m_items[m_highlightedItem];
            return { NULL, item->m_label, item->m_shortcut };
        }

        return { 0 };
    }

    void Render(DfWindow *win) {
        // Calculate how tall and wide the menu needs to be
        int height = GetHeight();
        int width = GetWidth();
        int x = m_left;
        int y = m_top + 1 * g_drawScale;

        // Draw the menu background
        int scale = RoundToInt(g_drawScale);
        DfDrawSunkenBox(win->bmp, x, y, width, height); // TODO - Shouldn't be sunken.
        RectFill(win->bmp, x + scale, y + scale, width - scale * 2, height - scale * 2, g_buttonColour);

        int ySize = GetItemHeight();

        // Highlight the selected item
        if (m_highlightedItem >= 0)
            RectFill(win->bmp, x + 2, m_items[m_highlightedItem]->m_top, width - 3, ySize, g_buttonHighlightColour);

        // Draw each menu item
        for (int i = 0; i < m_numItems; ++i) {
            MenuItem *item = m_items[i];

            if (item->IsSeparator()) {
                RectFill(win->bmp, x + 7, item->m_top + 4 - scale, // TODO - Apply scaling.
                    width - 10, scale, g_buttonShadowColour);
                RectFill(win->bmp, x + 7, item->m_top + 4,
                    width - 10, scale, g_buttonHighlightColour);
            }
            else {
                // Item label text
                int textY = item->m_top + MENU_ITEM_Y_SPACE * g_drawScale;
                DrawTextSimple(g_defaultFont, g_normalTextColour, win->bmp,
                    x + 10, textY, item->m_label);

                if (item->m_hotKeyIndex >= 0) {
                    int offset = GetTextWidthNumChars(g_defaultFont, item->m_label, item->m_hotKeyIndex);
                    int len = GetTextWidthNumChars(g_defaultFont, item->m_label + item->m_hotKeyIndex, 1);

                    // Underline hot key
                    int underscoreOffset = RoundToInt(g_defaultFont->charHeight * 0.88f);
                    int underscoreThickness = RoundToInt(g_drawScale * 1.2f);
                    RectFill(g_window->bmp, x + 10 + offset, textY + underscoreOffset,
                        len, underscoreThickness, g_normalTextColour);
                }

                // Shortcut
                if (item->m_shortcutDisplayString) {
                    DrawTextRight(g_defaultFont, g_normalTextColour, win->bmp,
                        x + width - 7, textY, item->m_shortcutDisplayString);
                }
            }
        }
    }

    static int GetItemHeight() {
        return g_defaultFont->charHeight + MENU_ITEM_Y_SPACE * 2 * g_drawScale;
    }

    int GetWidth() {
        int longest = 0;
        for (int i = 0; i < m_numItems; i++) {
            MenuItem *item = m_items[i];
            int width = GetTextWidth(g_defaultFont, item->m_label);
            if (width > longest) longest = width;
        }

        return longest + 30 * g_drawScale + m_longestShortcutLen;
    }

    int GetHeight() {
        MenuItem *item = m_items[m_numItems - 1];
        int rv = item->m_top - m_top + GetItemHeight();
        return rv;
    }

    bool IsMouseOverTitle(DfWindow *win, int menuBarHeight) {
        return !!DfMouseInRect(win, m_left, 0, m_titleWidth, menuBarHeight);
    }

    bool IsMouseOver(DfWindow *win) {
        int width = GetWidth();
        int height = GetHeight();
        if (DfMouseInRect(win, m_left, m_top, width, height))
            return true;
        return false;
    }

    int GetItemUnderMouse(DfWindow *win) {
        if (!IsMouseOver(win)) return -1;

        int ySize = GetItemHeight();
        for (int i = 0; i < m_numItems; i++) {
            MenuItem *item = m_items[i];
            if (item->IsSeparator()) continue;
        
            int y1 = item->m_top;
            int y2 = y1 + ySize;
            if (win->input.mouseY >= y1 && win->input.mouseY < y2) {
                return i;
            }
        }

        return -1;
    }

    void CalculateScreenPostions(int left, int top) {
        m_left = left;
        m_top = top;
        m_titleWidth = GetTextWidth(g_defaultFont, m_name) + MENU_ITEM_X_SPACE * 2 * g_drawScale;
        int itemTop = m_top + MENU_BAR_BOTTOM_SPACE * g_drawScale;
        int itemHeight = GetItemHeight();

        for (int i = 0; i < m_numItems; i++) {
            MenuItem *item = m_items[i];

            item->m_top = itemTop;
            if (item->IsSeparator())
                itemTop += itemHeight / 2;
            else
                itemTop += itemHeight;

            if (item->m_shortcutDisplayString) {
                int len = GetTextWidth(g_defaultFont, item->m_shortcutDisplayString);
                if (len > m_longestShortcutLen) {
                    m_longestShortcutLen = len;
                }
            }
        }

        // Adjust our y position to try to prevent us drawing off the bottom of
        // the screen. Mostly this is useful for context menus.
// TODO
//         int maxY = g_guiManager->m_height - m_height;
//         maxY = IntMax(0, maxY);
//         if (y > maxY) {
//             m_top = maxY;
//             CalculateScreenPostions();
//         }
    }

    MenuItem *FindMenuItemByHotkey(char c) {
        c = tolower(c);
        for (int i = 0; i < m_numItems; i++) {
            MenuItem *item = m_items[i];
            if (item->m_hotKeyIndex >= 0) {
                char hotkey = tolower(item->m_label[item->m_hotKeyIndex]);
                if (hotkey == c) {
                    return item;
                }
            }
        }

        return NULL;
    }
};


static void DrawTextWithAcceleratorKey(DfBitmap *bmp, int x, int y, char const *text, int accelKeyIdx, DfColour col) {
    DrawTextSimple(g_defaultFont, col, bmp, x, y, text);

    if (accelKeyIdx != -1) {
        int startX = GetTextWidthNumChars(g_defaultFont, text, accelKeyIdx);
        int endX = GetTextWidthNumChars(g_defaultFont, text, accelKeyIdx + 1);
        int yOffset = RoundToInt(g_defaultFont->charHeight * 0.88f);
        int height = RoundToInt(g_drawScale);
        RectFill(g_window->bmp, x + startX, y + yOffset, endX - startX, height, col);
    }
}


struct MenuBar {
    enum { MAX_MENUS = 10 };
    Menu *m_menus[MAX_MENUS];
    int m_numMenus;
    int m_height;
    Menu *m_highlightedMenu;     // NULL if no menu is highlighted
    bool m_displayed;            // True if the highlighted menu is displayed
    bool m_capturingInput;
    Menu *m_contextMenu;         // Null if there is no current context menu

    // Used to make alt up only focus menu bar if no input events have occurred
    // since alt down. Eg alt+drag. 
    // 0=alt down not seen since last alt up. This is the initial state.
    // 1=alt down seen, alt up NOT seen.
    // 2=alt down seen, other key/mouse events seen, alt up still NOT seen.
    int m_altState;

    MenuBar() {
        memset(this, 0, sizeof(MenuBar));
    }

    void CalculateScreenPositions() {
        m_height = (MENU_BAR_TOP_SPACE + MENU_ITEM_Y_SPACE * 2) * g_drawScale + g_defaultFont->charHeight;
        int x = MENU_BAR_LEFT_SPACE * g_drawScale;
        int y = m_height - 2 * g_drawScale;
        for (int i = 0; i < m_numMenus; i++) {
            Menu *menu = m_menus[i];
            menu->CalculateScreenPostions(x, y);
            x += menu->m_titleWidth;
        }
    }

    int GetHighlightedMenuIdx() {
        for (int i = 0; i < m_numMenus; i++) {
            if (m_menus[i] == m_highlightedMenu)
                return i;
        }

        return -1;
    }

    Menu *GetMenuTitleUnderMouseCursor(DfWindow *win) {
        for (int i = 0; i < m_numMenus; i++) {
            Menu *menu = m_menus[i];
            if (menu->IsMouseOverTitle(win, m_height)) {
                return menu;
            }
        }

        return NULL;
    }

    void ClearAllState() {
        m_highlightedMenu = NULL;
        m_displayed = false;
        m_capturingInput = false;
        m_contextMenu = NULL;
    }

    Menu *AddMenu(char const *title) {
        if (m_numMenus >= MAX_MENUS) return NULL;
        m_menus[m_numMenus] = new Menu(title);
        m_numMenus++;
        return m_menus[m_numMenus - 1];
    }

    void ShowContextMenu(Menu *contextMenu) {
        m_contextMenu = contextMenu;
        m_contextMenu->m_highlightedItem = 0;
        m_capturingInput = true;
    }

    DfGuiAction CheckKeyboardShortcuts(DfWindow *win) {
        for (int i = 0; i < m_numMenus; i++) {
            Menu *menu = m_menus[i];
            for (int j = 0; j < menu->m_numItems; j++) {
                MenuItem *item = menu->m_items[j];
                if (win->input.keyDowns[item->m_shortcut.key] &&
                    win->input.keys[KEY_CONTROL] == item->m_shortcut.ctrl &&
                    win->input.keys[KEY_SHIFT] == item->m_shortcut.shift &&
                    win->input.keys[KEY_ALT] == item->m_shortcut.alt) {
                    return { menu->m_name, item->m_label, item->m_shortcut };
                }
            }
        }

        return { 0 };
    }

    void DisplayMenu(Menu *menu, int highlightedItem) {
        m_highlightedMenu = menu;
        m_highlightedMenu->m_highlightedItem = highlightedItem;
        m_capturingInput = true;
        m_displayed = true;
    }

    DfGuiAction HandleEvents(DfWindow *win) {
        // The events and states of the menu system are described in
        // deadfrog-lib/docs/MenuStateMachine.png

        if (!win->input.windowHasFocus || win->input.keyDowns[KEY_ESC])
             ClearAllState();

        DfGuiAction event = { 0 };

        // Alt key up event.
        if (win->input.keyUps[KEY_ALT] && m_altState == 1) {
            if (m_capturingInput)
                ClearAllState();
            else {
                m_highlightedMenu = m_menus[0];
                m_capturingInput = true;
            }
        }

        // Alt+menu hotkey event.
        Menu *menuFromHotkey = FindMenuByHotKey(win);
        if (win->input.keys[KEY_ALT] && menuFromHotkey)
            DisplayMenu(menuFromHotkey, 0);

        // Hotkey typed event.
        if (win->input.numKeysTyped && m_capturingInput && !m_displayed)
            DisplayMenu(menuFromHotkey, 0);

        // Return key down event.
        if (win->input.keyDowns[KEY_ENTER] && m_capturingInput)
            DisplayMenu(m_highlightedMenu, 0);

        // Cursor left/right key down event.
        if (m_capturingInput) {
            int menuIndex = GetHighlightedMenuIdx();
            if (win->input.keyDowns[KEY_LEFT]) menuIndex--;
            if (win->input.keyDowns[KEY_RIGHT]) menuIndex++;
            menuIndex += m_numMenus;
            menuIndex %= m_numMenus;
            if (m_menus[menuIndex] != m_highlightedMenu) {
                m_highlightedMenu = m_menus[menuIndex];
                m_highlightedMenu->m_highlightedItem = 0;
            }
        }

        // Cursor up/down key down event.
        if ((win->input.keyDowns[KEY_UP] || win->input.keyDowns[KEY_DOWN]) &&
                m_capturingInput && !m_displayed)
            DisplayMenu(m_highlightedMenu, 0);

        // Mouse move event.
        bool mouseMoved = win->input.mouseVelX != 0 || win->input.mouseVelY != 0; 
        Menu *menuTitleUnderMouseCursor = GetMenuTitleUnderMouseCursor(win);
        if (mouseMoved && (menuTitleUnderMouseCursor || !m_capturingInput))
            m_highlightedMenu = menuTitleUnderMouseCursor;

        // Left mouse button down event.
        if (win->input.lmbClicked) {
            if (menuTitleUnderMouseCursor) {
                if (m_displayed) {
                    ClearAllState();
                }
                else {
                    DisplayMenu(menuTitleUnderMouseCursor, -1);
                }
            }
            else if (m_highlightedMenu && !m_highlightedMenu->IsMouseOver(win)) {
                ClearAllState();
            }
        }
        
        event = CheckKeyboardShortcuts(win);
        return event;
    }

    DfGuiAction Advance(DfWindow *win) {
        if (win->input.keys[KEY_ALT]) {
            if (m_altState == 0)
                m_altState = 1;
            else {
                int numKeyDowns = win->input.numKeyDowns;
                if (win->input.keyDowns[KEY_ALT])
                    numKeyDowns--;
                if (numKeyDowns || win->input.lmbClicked ||
                        win->input.mmbClicked || win->input.rmbClicked)
                    m_altState = 2;
            }
        }

        CalculateScreenPositions();

        DfGuiAction event = { 0 };
        if (m_highlightedMenu && m_displayed) {
            event = m_highlightedMenu->Advance(win);
            if (event.menuItemLabel) { // If not NULL event...
                event.menuName = m_highlightedMenu->m_name;
                ClearAllState();
            }
        }

        if (m_contextMenu) {
            if (win->input.lmbClicked || win->input.rmbClicked) {
                if (!m_contextMenu->IsMouseOver(win)) {
                    ClearAllState();
                }
            }
            else if (!event.menuItemLabel) {
                event = m_contextMenu->Advance(win);
                //event.menuName = ? // TODO
            }
        }

        if (!event.menuItemLabel)
            event = HandleEvents(win);

        if (win->input.keyUps[KEY_ALT])
            m_altState = 0;

        return event;
    }

    void Render(DfWindow *win) {
        // Draw the menu bar background
        RectFill(win->bmp, 0, 0, win->bmp->width, m_height, g_frameColour);

        // Draw the selection box around the highlighted menu
        if (m_highlightedMenu) {
            int boxTop = MENU_BAR_TOP_SPACE * g_drawScale;
            int boxHeight = Menu::GetItemHeight();
            RectFill(win->bmp, m_highlightedMenu->m_left, boxTop, 
                m_highlightedMenu->m_titleWidth, boxHeight, g_buttonHighlightColour);
        }

        // Draw the menu titles
        for (int i = 0; i < m_numMenus; i++) {
            Menu *menu = m_menus[i];
            int x = menu->m_left + MENU_ITEM_X_SPACE * g_drawScale;
            int y = (MENU_BAR_TOP_SPACE + MENU_ITEM_Y_SPACE) * g_drawScale;
            DrawTextWithAcceleratorKey(win->bmp, x, y, menu->m_name, 0, g_normalTextColour);
        }

        // Draw the displayed menu
        if (m_displayed)
            m_highlightedMenu->Render(win);

        if (m_contextMenu)
            m_contextMenu->Render(win);
    }

    Menu *FindMenuByName(char const *name) {
        for (int i = 0; i < m_numMenus; ++i) {
            if (strcasecmp(name, m_menus[i]->m_name) == 0) return m_menus[i];
        }

        return NULL;
    }

    Menu *FindMenuByHotKey(DfWindow *win) {
        for (int i = 0; i < m_numMenus; i++) {
            int keyId = m_menus[i]->m_name[0];
            if (win->input.keyDowns[keyId]) {
                return m_menus[i];
            }
        }

        return NULL;
    }
};


void DfMenuBarInit(DfMenuBar *dfMb) {
    MenuBar *mb = new MenuBar;
    dfMb->internals = mb;
}


void DfMenuBarAddAction(DfMenuBar *dfMb, char const *menuName, 
                        char const *menuItemLabel, DfKeyboardShortcut shortcut) {
    MenuBar *mb = (MenuBar*)dfMb->internals;
    mb->ClearAllState();
    Menu *menu = mb->FindMenuByName(menuName);
    if (!menu)
        menu = mb->AddMenu(menuName);
    
    if (menu)
        menu->AddItem(menuItemLabel, shortcut);
}


void DfMenuBarRemoveAction(DfMenuBar *dfMb, char const *menuName,
    char const *menuItemLabel, DfKeyboardShortcut shortcut) {
    MenuBar *mb = (MenuBar*)dfMb->internals;
    mb->ClearAllState();
    Menu *menu = mb->FindMenuByName(menuName);
    if (!menu)
        menu = mb->AddMenu(menuName);

    if (menu)
        menu->AddItem(menuItemLabel, shortcut);
}


DfGuiAction DfMenuBarDo(DfWindow *win, DfMenuBar *dfMb) {
    MenuBar *mb = (MenuBar*)dfMb->internals;
    DfGuiAction event = mb->Advance(win);
    mb->Render(win);
    dfMb->capturingInput = mb->m_capturingInput;
    dfMb->height = mb->m_height;
    return event;
}
