#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdarg.h>

#include "netget.h"
#include "irc_parser.h"

#define IRC_MTU 512

// for MOTD
#define RPL_MOTDSTART 375 // ":- <server> Message of the day - "
#define RPL_MOTD 372      // ":- <text>"
#define RPL_ENDOFMOTD 376 // ":End of /MOTD command"

// for NICK
#define ERR_NONICKNAMEGIVEN 431
#define ERR_ERRONEUSNICKNAME 432
#define ERR_NICKNAMEINUSE 433
#define ERR_NICKCOLLISION 436
#define ERR_UNAVAILRESOURCE 437

#define CHANNELS "#c-test,&c-test" // comma seperated "#1, #2, #3"

#define NICK_NAME "{manpage}"
#define USER_NAME "bot"
#define GECOS "Man Pages" // real name

char *g_nicks[] = {
    "man_bot",
    "_man_bot_",
    "{man_bot}",
    "{_manbot_}",
};

char *ChompWS(char *str);

int EstablishConnection(const char *host, const char *service_str);
int ConnectionRegistration(int sck);
int ConnectIRC(char *host, char *port);
int SendIrcMessage(int sck, char *fmt, ...);
void DisplayUntilFound(int sck, char *str);

int main(int argc, char **argv)
{
    char buf[8192];

    if (argc < 3)
    {
        printf("usage: ./bot server port\n");
        return 1;
    }

    int sck = ConnectIRC(argv[1], argv[2]);
    if (sck == 0)
    {
        printf("Failed to connect to server\n");
        return 1;
    }

    SendIrcMessage(sck, "JOIN %s\r\n", CHANNELS);

    return 0;
}

int ConnectIRC(char *host, char *port)
{
    int sck = EstablishConnection(host, port);

    if (sck == -1 || ConnectionRegistration(sck) == 0)
        return 0;

    return sck;
}

int SendIrcMessage(int sock, char *fmt, ...)
{
    char buf[IRC_MTU + 1];
    va_list ap;

    va_start(ap, fmt);
    int nprinted = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    ssize_t total_sent = 0;
    while (total_sent < nprinted)
    {
        ssize_t nsent = send(sock, buf, nprinted, 0);
        if (nsent < 0)
            return 0;
        total_sent += nsent;
    }

    return 1;
}

int ConnectionRegistration(int sck)
{
    if (SendIrcMessage(sck, "NICK %s\r\n", NICK_NAME) == 0)
        return 0;

    if (SendIrcMessage(sck, "USER %s * * :%s\r\n", USER_NAME, GECOS) == 0)
        return 0;

    return 1;
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

int EstablishConnection(const char *host, const char *service_str)
{
    struct addrinfo hints;
    struct addrinfo *listp = NULL;
    struct addrinfo *p = NULL;
    int status;
    int sck = -1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME | AI_ALL | AI_ADDRCONFIG;

    status = getaddrinfo(host, service_str, &hints, &listp);
    if (status != 0)
    {
        printf("getaddrinfo error: (%s)\n", gai_strerror(status));
        goto done;
    }

    for (p = listp; p; p = p->ai_next)
    {
        sck = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sck < 0)
            continue;

        struct timeval timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        if (setsockopt(sck, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
        {
            close(sck);
            goto done;
        }

        if (connect(sck, p->ai_addr, p->ai_addrlen) != -1)
            goto done;

        close(sck);
    }
    printf("all connects failed\n");

fail_closefd_freeaddr:
    close(sck);
done:
    if (listp != NULL)
        freeaddrinfo(listp);
    return sck;
}
