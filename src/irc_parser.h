#ifndef IRC_PARSE_H
#define IRC_PARSE_H

#define MAX_PARAMS 15

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

#endif // IRC_PARSE_H
