
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdarg.h>

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
int SendIrcMessage(int sck, char *fmt, ...);
void WaitAuth(int sck);

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
    int sck = EstablishConnection(host, port);

    WaitAuth(sck);
    if (sck == -1 || ConnectionRegistration(sck) == 0)
        return 0;

    return sck;
}

void WaitAuth(int sck)
{
    char line[8192];
    TEXTSCK stream;

    textsckinit(&stream, sck);

    netgets(line, sizeof(line), &stream); // NOTICE AUTH
    printf("%s", line);

    while (netgets(line, sizeof(line), &stream))
    {
        if (line[0] == '\r' && line[1] == '\n')
            break;

        printf("%s", line);
        if (strstr(line, "Found your hostname") != NULL)
            break;
    }
}

int SendIrcMessage(int sck, char *fmt, ...)
{
    int size = 0;
    char *p = NULL;
    va_list ap;

    va_start(ap, fmt);
    size = vsnprintf(p, size, fmt, ap);
    va_end(ap);

    if (size < 0)
        return 0;

    size++;
    p = malloc(size);
    if (p == NULL)
        return 0;

    va_start(ap, fmt);
    size = vsnprintf(p, size, fmt, ap);
    va_end(ap);

    if (size < 0)
    {
        free(p);
        return 0;
    }

    if (send(sck, p, strlen(p), 0) == -1)
    {
        printf("Failed to send message\n");
        return 0;
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
