#include <stdio.h>
#include <string.h>

#define MAX_PARAMS 15
#define MAX_MESSAGE 512

char *g_messages[] = {
    "PING :irc.Prison.NET\r\n",
    ":irc.Prison.NET 004 {TEST} irc.Prison.NET ircd-ratbox-2.2.9 oiwszcerkfydnxbauglZCD biklmnopstveI bkloveI\r\n",
    "NOTICE AUTH :*** Processing connection to irc.servercentral.net\r\n",
    "PING :irc.servercentral.net \r\n",
    ":irc.servercentral.net 372 {manpage} :- -  \r\n",
    ":irc.servercentral.net 376 {manpage} :End of /MOTD command.\r\n",
    ":{manpage}!~bot@d23-16-195-6.bchsia.telus.net JOIN :#c-test\r\n",
    ":irc.servercentral.net 353 {manpage} = #c-test :{manpage} tofu1\r\n",
    ":irc.servercentral.net 366 {manpage} #c-test :End of /NAMES list.\r\n",
    ":tofu1!~tofu@rootstorm.com PRIVMSG #c-test :test\r\n",
    ":tofu1!~tofu@rootstorm.com PRIVMSG {manpage} :message test\r\n",
    ":tofu1!~tofu@rootstorm.com PRIVMSG {manpage} :PING 1590092030 937335\r\n",
    ":tofu1!~tofu@rootstorm.com PRIVMSG {manpage} :VERSION\r\n",
    "::dude\\!~man@fe80::2 PRIVMSG {manpage} :message test\r\n",
    ":isTofu!~tofu@rootstorm.com PRIVMSG {manpage} :this is a test\r\n",
    NULL,
};

typedef struct
{
    char *message;
    char *prefix;
    char *command;
    char *params[MAX_PARAMS + 1];
    int num_params;
} IrcMessage;

char *ChompWS(char *str);
void PrintFields(IrcMessage msg);
int ParseIrcMessage(char *message_str, IrcMessage *msg);

int main(int argc, char **argv)
{
    IrcMessage msg;
    char **msg_ptr = g_messages;

    while (*msg_ptr != NULL)
    {
        char *new_msg = strdup(*msg_ptr);
        ParseIrcMessage(new_msg, &msg);
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

    msg->message = strdup(message_str);
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
