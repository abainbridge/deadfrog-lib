#pragma once

// Deadfrog lib headers.
#include "df_bitmap.h"
#include "df_window.h"

extern DfColour g_backgroundColour;
extern DfColour g_frameColour;
extern DfColour g_buttonShadowColour;
extern DfColour g_buttonHighlightColour;
extern DfColour g_normalTextColour;
extern DfColour g_selectionColour;

extern double g_drawScale;
extern int g_dragStartX;
extern int g_dragStartY;


// ****************************************************************************
// Misc functions
// ****************************************************************************

int DfMouseInRect(DfWindow *win, int x, int y, int w, int h);
void DfGuiDoFrame(DfWindow *win);

// The specified w and h is the external size of the box.
//
//        <-------- w -------->
//     ^  * * * * * * * * * * *  ^
//     |  * * * * * * * * * * *  | thickness
//     |  * * * * * * * * * * *  v
//     |  * * *           * * *
//   h |  * * *           * * *
//     |  * * *           * * *
//     |  * * * * * * * * * * *
//     |  * * * * * * * * * * *
//     v  * * * * * * * * * * *
//       <--->
//     thickness
void DfDrawSunkenBox(DfBitmap *bmp, int x, int y, int w, int h);


// ****************************************************************************
// V Scrollbar
// ****************************************************************************

typedef struct {
    int maximum;
    int currentVal;
    int coveredRange;
    int speed;

    int dragging;
    int currentValAtDragStart;
} DfVScrollbar;

int DfVScrollbarDo(DfWindow *win, DfVScrollbar *vs, int x, int y, int w, int h, int has_focus);


// ****************************************************************************
// Edit box
// ****************************************************************************

typedef struct {
    char text[128];
    double nextCursorToggleTime;
    char cursorOn;
    int cursorIdx;
} DfEditBox;


// Returns 1 if contents changed.
int DfEditBoxDo(DfWindow *win, DfEditBox *eb, int x, int y, int w, int h);


// ****************************************************************************
// List View
// ****************************************************************************

typedef struct {
    char const **items;
    int numItems;
    int selectedItem;
    int firstDisplayItem;
} DfListView;


// Returns id of item that was selected, or -1 if none were.
int DfListViewDo(DfWindow *win, DfListView *lv, int x, int y, int w, int h);


// ****************************************************************************
// Text View
// ****************************************************************************

enum { TEXT_VIEW_MAX_CHARS = 95000 };

typedef struct {
    DfVScrollbar v_scrollbar;
    char text[TEXT_VIEW_MAX_CHARS];
    char wrappedText[TEXT_VIEW_MAX_CHARS];
    
    // If start and end coords are the same, then there is no selection block.
    int selectionStartX;
    int selectionStartY;
    int selectionEndX;
    int selectionEndY;
} DfTextView;


void DfTextViewEmpty(DfTextView *tv);
void DfTextViewAddText(DfTextView *tv, char const *text);
void DfTextViewDo(DfWindow *win, DfTextView *tv, int x, int y, int w, int h);
char const *DfTextViewGetSelectedText(DfTextView *tv, int *num_chars);


// ****************************************************************************
// Button
// ****************************************************************************

typedef struct {
    char const *label;
} DfButton;


int DfButtonDo(DfWindow *win, DfButton *b, int x, int y, int w, int h);
