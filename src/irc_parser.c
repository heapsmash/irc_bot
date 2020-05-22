#include <stdio.h>
#include <string.h>

#include "irc_parser.h"
#include "util.h"

void PrintFields(IrcMessage msg)
{
    printf("[<prefix>: %s ] <command>: %s <params> ", msg.prefix, msg.command);
    for (int i = 0; i < msg.num_params; i++)
        printf("%s ", msg.params[i]);
}

// <message>  ::= [':' <prefix> <SPACE> ] <command> <params> <crlf>
int ParseIrcMessage(char *message_str, IrcMessage *msg)
{
    char *last = NULL;
    char *param;

    msg->message = ChompWS(strdup(message_str));
    msg->prefix = NULL;
    msg->num_params = 0;

    // If it has a prefix, parse that
    if (*message_str == ':')
    {
        msg->prefix = message_str + 1;

        message_str = strchr(msg->prefix, ' ');
        if (message_str == NULL)
            return 0;

        *message_str++ = '\0';
    }

    // Handle escaped string preemptively so we can use strtok_r in a portable manner
    char *trailing_param = strchr(message_str, ':');
    if (trailing_param)
    {
        *trailing_param++ = '\0';
        trailing_param = ChompWS(trailing_param);
    }

    // Tokenize command and params now
    msg->command = strtok_r(message_str, " ", &last);
    while ((param = strtok_r(NULL, " ", &last)) != NULL)
    {
        param = ChompWS(param);
        msg->params[msg->num_params++] = param;
    }

    // Add the trailing param to the end if there was one
    if (trailing_param)
        msg->params[msg->num_params++] = trailing_param;

    return 0;
}
