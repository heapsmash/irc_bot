#include <stdio.h>
#include <string.h>

#include "irc_parser.h"

char *ChompWS(char *str)
{
    str += strspn(str, " \t\r\n");

    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' ||
                         *end == '\r' || *end == '\n'))
        end--;

    end[1] = '\0';
    return str;
}

void PrintFields(IRC_MESSAGE_FIELDS msg)
{
    puts("\n########################");
    printf("<message>: \t%s\n", msg.message);
    puts("---------------");
    if (msg.prefix != NULL)
        printf("<prefix>: \t%s\n", msg.prefix);
    else
        printf("<prefix>: NULL\n");

    puts("---------------");
    printf("<command>: \t%s\n", msg.command);
    puts("---------------");

    printf("<params>:");
    for (int i = 0; msg.params[i] != NULL; i++)
        printf("\t\t%s\n", msg.params[i]);
    puts("\t\tNULL");
    puts("---------------");
    printf("<trailing>: \t%s\n", msg.trailing);

    puts("########################\n");
}

// <message>  ::= [':' <prefix> <SPACE> ] <command> <params> <crlf>
void ParseMessage(char *str, IRC_MESSAGE_FIELDS *msg)
{
    msg->message = strdup(str);

    if (str[0] == ':')
    {
        ParsePrefix(str, msg);
    }
    else // there was no prefix
    {
        msg->prefix = NULL;
        ParseCommand(str, msg);
    }
}

// <prefix>   ::= <servername> | <nick> [ '!' <user> ] [ '@' <host> ]
void ParsePrefix(char *str, IRC_MESSAGE_FIELDS *msg)
{
    msg->prefix = strtok(str, " ") + 1; // truncate :
    ParseCommand(NULL, msg);
}

// <command>  ::= <letter> { <letter> } | <number> <number> <number>
void ParseCommand(char *str, IRC_MESSAGE_FIELDS *msg)
{
    if (str == NULL)
    {
        msg->command = strtok(NULL, " ");
    }

    else // there was no prefix
    {
        msg->command = strtok(str, " ");
    }

    ParseParams(msg);
}

// <params>   ::= <SPACE> [ ':' <trailing> | <middle> <params> ]
void ParseParams(IRC_MESSAGE_FIELDS *msg)
{
    int i = -1;

    do
    {
        i++;
        msg->params[i] = strtok(NULL, " ");
        if (msg->params[i] != NULL)
        {
            char *contains_colon = strstr(msg->params[i], ":");
            if (contains_colon)
            {
                msg->params[i] = (strstr(msg->message, contains_colon)) + 1;
                msg->params[i] = ChompWS(msg->params[i]);
                msg->trailing = msg->params[i];
                msg->params[i + 1] = NULL;
                break;
            }
        }
    } while (msg->params[i] != NULL);
}
