#include <windows.h>
#include "lib/window_manager.h"
#include "message_dialog.h"


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
 		HWND hwnd = NULL;
// 		if (g_windowManager)
// 			hwnd = g_windowManager->m_hWnd;

        switch (MessageBox(hwnd, message, title, type))
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