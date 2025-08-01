// This module implements a message dialog box. It's just a wrapper for the local
// OS's standard message dialog box. It is mostly useful for showing error messages.

#pragma once


#ifdef __cplusplus
extern "C"
{
#endif


typedef struct _DfFont DfFont;


typedef enum {
    MsgDlgTypeYesNo,
    MsgDlgTypeYesNoCancel,
    MsgDlgTypeOk,
    MsgDlgTypeOkCancel
} MessageDialogType;


typedef enum {
    MsgDlgRtnCode_Fail = -1,
    MsgDlgRtnCode_Cancel,
    MsgDlgRtnCode_No,
    MsgDlgRtnCode_Ok,
    MsgDlgRtnCode_Retry,
    MsgDlgRtnCode_Yes
} MessageDialogReturnCode;


int MessageDialog(char const *title, char const *message, MessageDialogType type);

// Like the MessageDialog() but allows you to specify the font and where to 
// position the centre of the dialog on the desktop.
int MessageDialogEx(char const *title, char const *message, MessageDialogType type,
    DfFont *font, int centreX, int centreY);


#ifdef __cplusplus
}
#endif
