#include "df_message_dialog.h"

#include "fonts/df_prop.h"
#include "df_font.h"
#include "df_window.h"

#include <ctype.h> 
#include <stdlib.h>
#include <string.h>


struct Button {
    char const *label;
    int x, y, w, h;
};


static bool DoButton(DfWindow *win, DfFont *font, Button *button, bool isSelected) {
    DfColour light = { 0xffa0a0a0 };
    DfColour dark = { 0xff646464 };
    int x = button->x, y = button->y, w = button->w, h = button->h;
    int mx = win->input.mouseX;
    int my = win->input.mouseY;
    bool mouseOver = mx >= x && my >= y && mx < (x + w) && my < (y + h);

    RectOutline(win->bmp, x, y, w, h, dark);
    DfColour tlColour = g_colourWhite;
    DfColour brColour = light;
    if (win->input.lmb && mouseOver) {
        tlColour = light;
        brColour = g_colourWhite;
    }
    HLine(win->bmp, x + 1, y + 1, w - 3, tlColour);
    VLine(win->bmp, x + 1, y + 2, h - 4, tlColour);
    HLine(win->bmp, x + 2, y+h-2, w - 4, brColour);
    VLine(win->bmp, x+w-2, y + 2, h - 3, brColour);

    int textY = y + h / 2 - font->charHeight / 2;
    int textWidth = GetTextWidth(font, button->label);
    int buttonCentreX = x + w / 2;
    int labelX = buttonCentreX - textWidth / 2;
    int accelKeyUnderlineY = textY + font->charHeight * 0.88;
    int accelKeyUnderlineWidth = font->charHeight * 0.55;
    DrawTextSimple(font, g_colourBlack, win->bmp, labelX, textY, button->label);
    HLine(win->bmp, labelX, accelKeyUnderlineY, accelKeyUnderlineWidth, g_colourBlack);

    if (isSelected)
        RectOutline(win->bmp, button->x + 4, button->y + 4, button->w - 8, button->h - 8, light);

    if (win->input.lmbUnClicked && mouseOver)
        return true;
    if (isSelected && win->input.keyDowns[KEY_ENTER])
        return true;
    if (win->input.numKeysTyped > 0 && 
        win->input.keysTyped[0] == tolower(button->label[0]))
        return true;

    return false;
}


int MessageDialog(char const *title, char const *message, MessageDialogType type) {
    DfFont *font = LoadFontFromMemory(df_prop_8x15, sizeof(df_prop_8x15));
    int rv = MessageDialogEx(title, message, type, font, -1, -1);
    FontDelete(font);
    return rv;
}


int MessageDialogEx(char const *title, char const *message, MessageDialogType type,
        DfFont *font, int centreX, int centreY) {
    int numLines = 0;
    int longestLinePixels = 0;
    {
        char const *lineStart = message;
        for (char const *c = message;; c++) {
            if (*c == '\n' || *c == '\0') {
                int lineLen = GetTextWidthNumChars(font, lineStart, c - lineStart);
                if (lineLen > longestLinePixels) longestLinePixels = lineLen;
                lineStart = c + 1;
                numLines++;
            }
            if (*c == '\0') break;
        }
    }

    int textSpaceX = font->charHeight * 1.5;
    int textSpaceY = font->charHeight * 2;
    int buttonHeight = font->charHeight * 2;
    int buttonBarHeight = buttonHeight * 2;
    int buttonBarTop = textSpaceY * 2 + numLines * font->charHeight;
    int buttonTop = buttonBarTop + font->charHeight;
    int buttonWidth = font->charHeight * 6;
    DfColour buttonColour = { 0xffe0e0e0 };

    Button buttons[3];
    int numButtons = 1;
    switch (type) {
    case MsgDlgTypeYesNo: numButtons = 2; buttons[0].label = "Yes"; buttons[1].label = "No"; break;
    case MsgDlgTypeYesNoCancel: numButtons = 3; buttons[0].label = "Yes"; buttons[1].label = "No"; buttons[2].label = "Cancel"; break;
    case MsgDlgTypeOk: numButtons = 1; buttons[0].label = "OK"; break;
    case MsgDlgTypeOkCancel: numButtons = 2; buttons[0].label = "OK"; buttons[1].label = "Cancel"; break;
    }

    int widthNeededForButtons = textSpaceX * (3 + numButtons) + buttonWidth * numButtons;
    int winWidth = textSpaceX * 2 + longestLinePixels;
    if (widthNeededForButtons > winWidth)
        winWidth = widthNeededForButtons;
    int winHeight = buttonBarTop + buttonBarHeight;

    // Place buttons, right-most first.
    {
        int x = winWidth - textSpaceX * 2 - buttonWidth;
        for (int i = numButtons - 1; i > -1; i--) {
            buttons[i].x = x;
            buttons[i].y = buttonTop;
            buttons[i].w = buttonWidth;
            buttons[i].h = buttonHeight;
            x -= buttonWidth + textSpaceX;
        }
    }

    DfWindow *win;
    if (centreX == -1 && centreY == -1) {
        win = CreateWin(winWidth, winHeight, WT_WINDOWED_FIXED, title);
    }
    else {
        int posX = centreX - winWidth / 2;
        int posY = centreY - winHeight / 2;
        win = CreateWinPos(posX, posY, winWidth, winHeight, WT_WINDOWED_FIXED, title);
    }

    int selectedButton = 0;
    int result = -1;
    while (!win->windowClosed && !win->input.keys[KEY_ESC] && result == -1) {
        InputPoll(win);
        RectFill(win->bmp, 0, 0, winWidth, buttonBarTop, g_colourWhite);

        unsigned x = textSpaceX;
        unsigned y = textSpaceY;
        char const *lineStart = message;
        while (1) {
            char const *lineEnd = strchr(lineStart, '\n');
            if (!lineEnd) lineEnd = strchr(lineStart, '\0');
            DrawTextSimpleLen(font, g_colourBlack, win->bmp, x, y, lineStart, lineEnd - lineStart);
            if (*lineEnd == '\0') break;
            lineStart = lineEnd + 1;
            y += font->charHeight;
        }

        RectFill(win->bmp, 0, buttonBarTop, winWidth, buttonBarHeight, buttonColour);

        if (win->input.keyDowns[KEY_LEFT] ||
            (win->input.keyDowns[KEY_TAB] && win->input.keys[KEY_SHIFT])) {
            selectedButton = (selectedButton + numButtons - 1) % numButtons;
        }
        if (win->input.keyDowns[KEY_RIGHT] ||
            (win->input.keyDowns[KEY_TAB] && !win->input.keys[KEY_SHIFT])) {
            selectedButton = (selectedButton + 1) % numButtons;
        }

        for (int i = 0; i < numButtons; i++) {
            bool clicked = DoButton(win, font, &buttons[i], i == selectedButton);
            if (clicked) result = i;
        }

        UpdateWin(win);
        WaitVsync();
    }

    DestroyWin(win);

    switch (type) {
    case MsgDlgTypeYesNo:
    case MsgDlgTypeYesNoCancel:
        switch (result) {
        case 0: return MsgDlgRtnCode_Yes;
        case 1: return MsgDlgRtnCode_No;
        case 2: return MsgDlgRtnCode_Cancel;
        }
        break;
    case MsgDlgTypeOk:
    case MsgDlgTypeOkCancel: 
        switch (result) {
        case 0: return MsgDlgRtnCode_Ok;
        case 1: return MsgDlgRtnCode_Cancel;
        }
        break;
    }

    return MsgDlgRtnCode_Cancel;
}
