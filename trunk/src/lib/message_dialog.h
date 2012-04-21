#ifndef INCLUDED_MESSAGE_BOX_H
#define INCLUDED_MESSAGE_BOX_H


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


#endif
