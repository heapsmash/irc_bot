
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <fcntl.h>

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

#define CHANNELS "#c" // comma seperated "#1, #2, #3"

#define NICK_NAME "{man_bot}"
#define USER_NAME "bot"
#define GECOS "Man Pages" // real name

char *g_nicks[] = {
    "man_bot",
    "_man_bot_",
    "{man_bot}",
    "{_manbot_}",
};

typedef struct
{
    char *key;
    char *val;
} KVPAIR;

typedef struct
{
    int fd;
    int is_done;
    size_t pos;
    size_t buflen;
    char buf[BUFSIZ];
} TEXTSCK;

char *ChompWS(char *str);
void textsckinit(TEXTSCK *stream, int fd);
char *netgets(char *str, size_t size, TEXTSCK *stream);
int EstablishConnection(const char *host, const char *service_str);
int ConnectionRegistration(int sck);
int ConnectIRC(char *host, char *port);
void GetMOTD(int fd);

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

    //// Establish QUIT.
    snprintf(buf, sizeof(buf), "QUIT");
    if (send(sck, buf, strlen(buf), 0) == -1)
    {
        printf("Failed to QUIT\n");
    }

    return 0;
}

int ConnectIRC(char *host, char *port)
{
    char buf[8192];
    int sck = EstablishConnection(host, port);

    if (sck == -1 || ConnectionRegistration(sck) == 0)
        return 0;

    puts("GOT TO GetMOTD");
    GetMOTD(sck);
    puts("OUT OF MOTD");

    return sck;
}

void GetMOTD(int sck)
{
    TEXTSCK stream;
    char buf[8192];

    textsckinit(&stream, sck);

    while (netgets(buf, sizeof buf, &stream))
    {
        printf("%s", buf);
    }
}

int ConnectionRegistration(int sck)
{
    char buf[8192];

    //// Establish NICK.
    snprintf(buf, sizeof(buf), "NICK %s", NICK_NAME);
    if (send(sck, buf, strlen(buf), 0) == -1)
    { // TODO: Handle nick in use case
        printf("Failed to Establish NICK\n");
        return 0;
    }

    //// Establish USER.
    snprintf(buf, sizeof(buf), "USER %s * * :%s", USER_NAME, GECOS);
    if (send(sck, buf, strlen(buf), 0) == -1)
    {
        printf("Failed to Establish USER\n");
        return 0;
    }

    return 1;
}

void textsckinit(TEXTSCK *stream, int fd)
{
    stream->fd = fd;
    stream->is_done = 0;
    stream->pos = 0;
    stream->buflen = 0;
}

int netgetc(TEXTSCK *stream)
{
    if (stream->is_done)
        return EOF;

    if (stream->pos == stream->buflen)
    {
        ssize_t nread = read(stream->fd, stream->buf, sizeof(stream->buf));
        if (nread <= 0)
            stream->is_done = 1;

        stream->pos = 0;
        stream->buflen = nread;
    }

    return stream->buf[stream->pos++];
}

char *netgets(char *str, size_t size, TEXTSCK *stream)
{
    char *s = str;
    int c = 0;

    if (stream->is_done)
        return NULL;

    for (size_t i = 0; i != size - 1 && c != '\n'; i++)
    {
        c = netgetc(stream);
        if (c == EOF)
            break;

        *s++ = c;
    }

    *s = '\0';
    return str;
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
