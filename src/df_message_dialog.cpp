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


static bool DoButton(DfWindow *win, DfFont *font, int x, int y, int w, int h, char const *text)
{
    DfColour light = { 0xffe3e3e3 };
    DfColour medium = { 0xffa0a0a0 };
    DfColour dark = { 0xff646464 };
    RectOutline(win->bmp, x, y, w, h, dark);
    HLine(win->bmp, x + 1, y + 1, w - 3, g_colourWhite);
    VLine(win->bmp, x + 1, y + 2, h - 4, g_colourWhite);
    HLine(win->bmp, x + 2, y+h-2, w - 4, medium);
    VLine(win->bmp, x+w-2, y + 2, h - 3, medium);

    int textY = y + h / 2 - font->charHeight / 2;
    DrawTextCentre(font, g_colourBlack, win->bmp, x + w/2, textY, text);

    int mx = win->input.mouseX;
    int my = win->input.mouseY;
    if (win->input.lmbUnClicked && mx >= x && my >= y && mx < (x + w) && my < (y + h)) {
        return true;
    }
    return false;
}


int MessageDialog(char const *title, char const *message, MessageDialogType type)
{
    DfFont *font = LoadFontFromMemory(df_prop_8x15, sizeof(df_prop_8x15));

    int numLines = 0;
    int longestLinePixels = 0;
    {
        char const *lineStart = message;
        for (char const *c = message; ; c++) {
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

    DfColour buttonColour = { 0xffe0e0e0 };

    int winWidth = textSpaceX * 2 + longestLinePixels;
    int winHeight = buttonBarTop + buttonBarHeight;

    DfWindow *win = CreateWin(winWidth, winHeight, WT_WINDOWED, title);

    while (!win->windowClosed && !win->input.keys[KEY_ESC])
    {
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
        bool clicked = DoButton(win, font, 50, buttonTop, winWidth / 2, buttonHeight, "OK");
        if (clicked) break;

        UpdateWin(win);
        WaitVsync();
    }

    DestroyWin(win);

    exit(0);
    return 0;
}

#endif
