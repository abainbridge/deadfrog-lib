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

// Updates draw scale and g_defaultFont, records start of drag events.
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
	int selectionIdx; // Equal to cursorIdx if no selection.
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

enum { DF_TEXT_VIEW_MAX_CHARS = 95000 };

typedef struct {
    DfVScrollbar vScrollbar;
    char text[DF_TEXT_VIEW_MAX_CHARS];
    char wrappedText[DF_TEXT_VIEW_MAX_CHARS];
    
    // If start and end coords are the same, then there is no selection block.
    int selectionStartX;
    int selectionStartY;
    int selectionEndX;
    int selectionEndY;
} DfTextView;


void DfTextViewEmpty(DfTextView *tv);
void DfTextViewAddText(DfTextView *tv, char const *text);
void DfTextViewDo(DfWindow *win, DfTextView *tv, int x, int y, int w, int h);
char const *DfTextViewGetSelectedText(DfTextView *tv, int *numChars);


// ****************************************************************************
// Button
// ****************************************************************************

typedef struct {
    char const *label;
} DfButton;


int DfButtonDo(DfWindow *win, DfButton *b, int x, int y, int w, int h);


// ****************************************************************************
// Menu Bar and Keyboard Shortcuts
// ****************************************************************************

typedef struct {
    unsigned char key; // eg KEY_F1. Equals zero if shortcut is empty/null/unused.
    unsigned ctrl : 1;
    unsigned shift : 1;
    unsigned alt : 1;
    // TODO add isGlobal.
} DfKeyboardShortcut;

typedef struct {
    char const *menuName; // NULL if not on a menu.
    char const *menuItemLabel; // NULL if not on a menu.
    DfKeyboardShortcut shortcut; // All zero if there is no shortcut.
} DfGuiAction;

typedef struct {
    int height; // Is updated by DfMenuDo(). Use this to determine the remaining space in your window.
    int capturingInput; // If this is true, other widgets should ignore keyboard and mouse input.

    void *internals;
} DfMenuBar;


void DfMenuBarInit(DfMenuBar *mb);

// Pass menuName=NULL if you don't want a menu entry. 
// Pass menuItemLabel as NULL if you want to add a separator to a menu.
// Pass shortcut as all zeros if you don't want a keyboard shortcut.
void DfMenuBarAddAction(DfMenuBar *mb, char const *menuName, char const *menuItemLabel, DfKeyboardShortcut shortcut);

// If menu item was selected, or its keyboard shortcut was activated, then that menu item is returned.
DfGuiAction DfMenuBarDo(DfWindow *win, DfMenuBar *menu);
