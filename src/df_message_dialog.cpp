#include "df_message_dialog.h"

#if 1
#include <windows.h>

int MessageDialog(char const *title, char const *message, MessageDialogType type)
{
    switch (type)
    {
        case MsgDlgTypeYesNo:       type = (MessageDialogType)MB_YESNO; break;
        case MsgDlgTypeYesNoCancel: type = (MessageDialogType)MB_YESNOCANCEL; break;
        case MsgDlgTypeOk:          type = (MessageDialogType)MB_OK; break;
        case MsgDlgTypeOkCancel:    type = (MessageDialogType)MB_OKCANCEL; break;
    }

    int rv = -1;
    if (type != -1)
    {
        switch (MessageBox(NULL, message, title, type))
        {
            case IDABORT: rv = MsgDlgRtnCode_Abort; break;
            case IDCANCEL: rv = MsgDlgRtnCode_Cancel; break;
            case IDNO: rv = MsgDlgRtnCode_No; break;
            case IDOK: rv = MsgDlgRtnCode_Ok; break;
            case IDRETRY: rv = MsgDlgRtnCode_Retry; break;
            case IDYES: rv = MsgDlgRtnCode_Yes; break;
        }
    }

    return rv;
}

#else

#include "fonts/df_prop.h"
#include "df_font.h"
#include "df_window.h"

#include <stdlib.h>
#include <string.h>


struct Button
{
    char const *label;
    int x, y, w, h;
};


static bool DoButton(DfWindow *win, DfFont *font, Button *button)
{
    DfColour light = { 0xffe3e3e3 };
    DfColour medium = { 0xffa0a0a0 };
    DfColour dark = { 0xff646464 };
    int x = button->x, y = button->y, w = button->w, h = button->h;
    RectOutline(win->bmp, x, y, w, h, dark);
    HLine(win->bmp, x + 1, y + 1, w - 3, g_colourWhite);
    VLine(win->bmp, x + 1, y + 2, h - 4, g_colourWhite);
    HLine(win->bmp, x + 2, y+h-2, w - 4, medium);
    VLine(win->bmp, x+w-2, y + 2, h - 3, medium);

    int textY = y + h / 2 - font->charHeight / 2;
    DrawTextCentre(font, g_colourBlack, win->bmp, x + w/2, textY, button->label);

    int mx = win->input.mouseX;
    int my = win->input.mouseY;
    if (win->input.lmbUnClicked && mx >= x && my >= y && mx < (x + w) && my < (y + h)) {
        return true;
    }
    if (win->input.keyDowns[KEY_ENTER]) {
        return true;
    }
    return false;
}


int MessageDialog(char const *title, char const *message, MessageDialogType type) {
    type = MsgDlgTypeYesNoCancel;
    DfFont *font = LoadFontFromMemory(df_prop_8x15, sizeof(df_prop_8x15));

    int numLines = 0;
    int longestLinePixels = 0;
    {
        char const *lineStart = message;
        for (char const *c = message;; c++) {
            if (*c == '\n' || *c == '\0') {
                int lineLen = GetTextWidth(font, lineStart, c - lineStart);
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
    int numButtons;
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

    DfWindow *win = CreateWin(winWidth, winHeight, WT_WINDOWED, title);

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
        
        for (int i = 0; i < numButtons; i++) {
            bool clicked = DoButton(win, font, &buttons[i]);
            if (clicked) result = i;
        }

        UpdateWin(win);
        WaitVsync();
    }

    DestroyWin(win);
    FontDelete(font);

    exit(0);
    return result;
}

#endif
