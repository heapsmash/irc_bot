#ifndef IRC_PARSE_H
#define IRC_PARSE_H

#define MAX_PARAMS 15

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

#endif // IRC_PARSE_H
