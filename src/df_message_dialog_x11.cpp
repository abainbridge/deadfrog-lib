#include "df_message_dialog.h"
#include <stdio.h>


int MessageDialog(char const *title, char const *message, MessageDialogType type)
{
    printf("%s\n", message);
    return 0;
}
