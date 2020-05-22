#include <stdio.h>
#include <string.h>

#define MAX_PARAMS 15
#define MAX_MESSAGE 512

char *g_messages[] = {
    "PING :irc.Prison.NET",
    ":irc.Prison.NET 004 {TEST} irc.Prison.NET ircd-ratbox-2.2.9 oiwszcerkfydnxbauglZCD biklmnopstveI bkloveI",
    "NOTICE AUTH :*** Processing connection to irc.servercentral.net",
    "PING :irc.servercentral.net ",
    ":irc.servercentral.net 372 {manpage} :- -  ",
    ":irc.servercentral.net 376 {manpage} :End of /MOTD command.",
    ":{manpage}!~bot@d23-16-195-6.bchsia.telus.net JOIN :#c-test",
    ":irc.servercentral.net 353 {manpage} = #c-test :{manpage} tofu1",
    ":irc.servercentral.net 366 {manpage} #c-test :End of /NAMES list.",
    ":tofu1!~tofu@rootstorm.com PRIVMSG #c-test :test",
    ":tofu1!~tofu@rootstorm.com PRIVMSG {manpage} :message test",
    ":tofu1!~tofu@rootstorm.com PRIVMSG {manpage} :PING 1590092030 937335",
    ":tofu1!~tofu@rootstorm.com PRIVMSG {manpage} :VERSION",
    "::dude\\!~man@fe80::2 PRIVMSG {manpage} :message test",
    ":isTofu!~tofu@rootstorm.com PRIVMSG {manpage} :this is a test",
    NULL,
};

typedef struct _IRC_MESSAGE_FIELDS
{
    char *message;
    char *trailing;
    char *prefix;
    char *command;
    char *params[MAX_PARAMS + 1];
} IRC_MESSAGE_FIELDS;

void ParseMessage(char *str, IRC_MESSAGE_FIELDS *msg);
void ParsePrefix(char *str, IRC_MESSAGE_FIELDS *msg);
void ParseCommand(char *str, IRC_MESSAGE_FIELDS *msg);
void ParseParams(IRC_MESSAGE_FIELDS *msg);
void PrintFields(IRC_MESSAGE_FIELDS msg);
char *ChompWS(char *str);

int main(int argc, char **argv)
{
    IRC_MESSAGE_FIELDS msg;
    char **msg_ptr = g_messages;

    while (*msg_ptr != NULL)
    {
        char *new_msg = strdup(*msg_ptr);
        ParseMessage(new_msg, &msg);
        PrintFields(msg);
        msg_ptr++;
    }

    return 0;
}

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

    printf("<params>:\t");
    for (int i = 0; msg.params[i] != NULL; i++)
        printf("%s,", msg.params[i]);
    puts("NULL");
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
            if (msg->params[i][0] == ':')
            {
                msg->params[i] = (strstr(msg->message, &msg->params[i][0])) + 1;
                msg->params[i] = ChompWS(msg->params[i]);
                msg->trailing = msg->params[i];
                msg->params[i + 1] = NULL;
                break;
            }
        }

    } while (msg->params[i] != NULL);
}
