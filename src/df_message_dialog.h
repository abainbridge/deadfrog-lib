// This module implements a message dialog box. It's just a wrapper for the local
// OS's standard message dialog box. It is mostly useful for showing error messages.

#pragma once


enum MessageDialogType
{
	MsgDlgTypeYesNo,
	MsgDlgTypeYesNoCancel,
	MsgDlgTypeOk,
	MsgDlgTypeOkCancel
};


enum MessageDialogReturnCode
{
    MsgDlgRtnCode_Fail = -1,
    MsgDlgRtnCode_Abort,
    MsgDlgRtnCode_Cancel,
    MsgDlgRtnCode_No,
    MsgDlgRtnCode_Ok,
    MsgDlgRtnCode_Retry,
    MsgDlgRtnCode_Yes
};


int MessageDialog(char const *title, char const *message, MessageDialogType type);

