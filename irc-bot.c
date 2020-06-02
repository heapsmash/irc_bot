/* compile with gcc irc-bot.c `pkg-config --cflags --libs openssl` -o irc-bot */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <openssl/ssl.h>

#define READ 0
#define WRITE 1

int SendIrcLine(int fd, const char *fmt, ...);
int SendSslIrcMsg(SSL *ssl, const char *fmt, ...);

char *ChompWS(char *str);
int EstablishConnection(const char *host, const char *service_str);
SSL *InitSslOnSocket(int sckfd);
FILE *my_popen(char *cmdstring);

typedef struct
{
    char *message;
    char *prefix;
    char *command;
    char *params[16];
    int num_params;
} IrcMessage;

typedef struct
{
    int sckfd;
    SSL *ssl;
} IrcSession;

int ParseIrcMessage(char *message_str, IrcMessage *msg);
void HandleIrcMessage(int sck, IrcMessage *msg);

typedef struct
{
    int fd;
    int is_done;
    SSL *ssl;
    size_t pos;
    size_t buflen;
    char buf[BUFSIZ];
} TEXTSCK;

void textsckinit(TEXTSCK *stream, int fd, SSL *ssl);
int netgetc(TEXTSCK *stream);
char *netgets(char *str, size_t size, TEXTSCK *stream);
void exec(SSL *ssl, char *command, char *nick);

int main(int argc, char *argv[])
{
    const char *host;
    const char *port;
    const char *password;

    if (argc < 4)
    {
        host = "irc.servercentral.net";
        port = "6667";
    }

    else
    {
        host = argv[1];
        port = argv[2];
        password = argv[3];
    }

    int sck = EstablishConnection(host, port);
    if (sck == -1)
    {
        printf("Failed to create socket\n");
        return 1;
    }

    SSL *ssl = InitSslOnSocket(sck);
    if (ssl == NULL)
    {
        printf("Failed to initialize SSL\n");
        return 1;
    }

    const char *nickname = "{manpage}";
    const char *username = "manpage";
    const char *gecos = "man pages";

    SendSslIrcMsg(ssl, "PASS %s\r\n", password);
    SendSslIrcMsg(ssl, "NICK %s\r\n", nickname);
    SendSslIrcMsg(ssl, "USER %s * * :%s\r\n", username, gecos);
    SendSslIrcMsg(ssl, "JOIN #c-learning\r\n");

    char buf[513];
    TEXTSCK stream;

    textsckinit(&stream, sck, ssl);

    while (netgets(buf, sizeof(buf), &stream)) // all of this is disgusting and needs to be fixed... 
    {
        printf("%s", buf);
        IrcMessage msg;
        ParseIrcMessage(buf, &msg);
        HandleIrcMessage(sck, &msg);

        if (msg.prefix == NULL && strncmp(msg.command, "PING", strlen(msg.command) - 1) == 0)
            SendSslIrcMsg(ssl, "PONG %s\r\n", msg.params[0]);

        if (strstr(msg.command, "KICK") && strstr(msg.params[1], "{manpage}"))
        {
            SendSslIrcMsg(ssl, "JOIN %s\r\n", msg.params[0]);
        }

        char *check_bot_commands = msg.params[msg.num_params - 1];

        if (check_bot_commands != NULL)
        {
            int command_len = strlen(check_bot_commands + 3);
            if (command_len > 20)
                continue;

            char *check_command_ptr = strdup(check_bot_commands);

            check_command_ptr = strtok(check_command_ptr, " ");
            if (strlen(check_command_ptr) == 4 && strstr(check_bot_commands, "!man"))
            {
                check_bot_commands = strtok(check_bot_commands, "!");
                check_bot_commands = strtok(check_bot_commands, "@");
                check_bot_commands = strtok(check_bot_commands, "#");
                check_bot_commands = strtok(check_bot_commands, "$");
                check_bot_commands = strtok(check_bot_commands, "^");
                check_bot_commands = strtok(check_bot_commands, "&");
                check_bot_commands = strtok(check_bot_commands, "*");
                check_bot_commands = strtok(check_bot_commands, "(");
                check_bot_commands = strtok(check_bot_commands, ")");
                check_bot_commands = strtok(check_bot_commands, "_");
                check_bot_commands = strtok(check_bot_commands, "-");
                check_bot_commands = strtok(check_bot_commands, "=");
                check_bot_commands = strtok(check_bot_commands, "+");
                check_bot_commands = strtok(check_bot_commands, "~");
                check_bot_commands = strtok(check_bot_commands, "`");
                check_bot_commands = strtok(check_bot_commands, ":");
                check_bot_commands = strtok(check_bot_commands, ";");
                check_bot_commands = strtok(check_bot_commands, "'");
                check_bot_commands = strtok(check_bot_commands, "|");
                check_bot_commands = strtok(check_bot_commands, "\\");
                check_bot_commands = strtok(check_bot_commands, "\"");
                check_bot_commands = strtok(check_bot_commands, "/");
                check_bot_commands = strtok(check_bot_commands, ">");
                check_bot_commands = strtok(check_bot_commands, "<");
                check_bot_commands = strtok(check_bot_commands, ",");
                check_bot_commands = strtok(check_bot_commands, ".");
                check_bot_commands = strtok(check_bot_commands, "?");

                char *user = strtok(msg.prefix, "!");

                char cmdstring[50 + sizeof "man -P cat"] = "man -P cat";
                char *cmd_ptr = strncat(cmdstring, check_bot_commands + 3, 50 + sizeof "man -P cat" - 1);

                SendSslIrcMsg(ssl, "PRIVMSG %s :Sending (%s) %s\r\n", msg.params[0], user, cmd_ptr);

                exec(ssl, cmd_ptr, user);
            }

            else if (strlen(check_command_ptr) == 8 && strstr(check_bot_commands, "!apropos"))
            {
                check_bot_commands = strtok(check_bot_commands, "!");
                check_bot_commands = strtok(check_bot_commands, "@");
                check_bot_commands = strtok(check_bot_commands, "#");
                check_bot_commands = strtok(check_bot_commands, "$");
                check_bot_commands = strtok(check_bot_commands, "^");
                check_bot_commands = strtok(check_bot_commands, "&");
                check_bot_commands = strtok(check_bot_commands, "*");
                check_bot_commands = strtok(check_bot_commands, "(");
                check_bot_commands = strtok(check_bot_commands, ")");
                check_bot_commands = strtok(check_bot_commands, "_");
                check_bot_commands = strtok(check_bot_commands, "-");
                check_bot_commands = strtok(check_bot_commands, "=");
                check_bot_commands = strtok(check_bot_commands, "+");
                check_bot_commands = strtok(check_bot_commands, "~");
                check_bot_commands = strtok(check_bot_commands, "`");
                check_bot_commands = strtok(check_bot_commands, ":");
                check_bot_commands = strtok(check_bot_commands, ";");
                check_bot_commands = strtok(check_bot_commands, "'");
                check_bot_commands = strtok(check_bot_commands, "|");
                check_bot_commands = strtok(check_bot_commands, "\\");
                check_bot_commands = strtok(check_bot_commands, "/");
                check_bot_commands = strtok(check_bot_commands, ">");
                check_bot_commands = strtok(check_bot_commands, "<");
                check_bot_commands = strtok(check_bot_commands, ",");
                check_bot_commands = strtok(check_bot_commands, ".");
                check_bot_commands = strtok(check_bot_commands, "?");
                check_bot_commands = strtok(check_bot_commands, "\"");
                check_bot_commands = strtok(check_bot_commands, "{");
                check_bot_commands = strtok(check_bot_commands, "}");

                char *user = strtok(msg.prefix, "!");
                SendSslIrcMsg(ssl, "PRIVMSG %s :Sending (%s) %s\r\n", msg.params[0], user, check_bot_commands);

                exec(ssl, check_bot_commands, user);
            }
            else if (strlen(check_command_ptr) == 9 && strstr(check_bot_commands, "!commands"))
            {
                char *user = strtok(msg.prefix, "!");
                SendSslIrcMsg(ssl, "PRIVMSG %s :%s My commands are !apropos and !man\r\n", msg.params[0], user);
            }
        }
    }

    return 0;
}

void exec(SSL *ssl, char *command, char *chan)
{
    char buf[1024] = "";

    FILE *sys = my_popen(command);
    while (fgets(buf, sizeof(buf), sys))
    {
        SendSslIrcMsg(ssl, "PRIVMSG %s :%s\n", chan, buf);
        memset(buf, 0, sizeof(buf));
    }

    fclose(sys);
}

FILE *my_popen(char *cmdstring)
{
    int pipefd[2];

    char buf[32], *argv[] = {"/bin/sh", "-c", cmdstring, NULL};

    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork()");
        return NULL;
    }

    if (pid == 0) // child
    {
        int out_fd = dup2(pipefd[WRITE], STDOUT_FILENO);
        if (out_fd < 0)
            perror("dup2");

        close(pipefd[WRITE]);

        execv("/bin/sh", argv);
        perror("execve failed!");
        _exit(123);
    }

    int wstatus;

    if (pid != wait(&wstatus) || !WIFEXITED(wstatus))
    {
        perror("Divorsed");
        return NULL;
    }

    close(pipefd[WRITE]);

    return fdopen(pipefd[READ], "r");
}

void textsckinit(TEXTSCK *stream, int fd, SSL *ssl)
{
    stream->ssl = ssl;
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
        ssize_t nread = stream->ssl ? SSL_read(stream->ssl, stream->buf, sizeof(stream->buf)) : read(stream->fd, stream->buf, sizeof(stream->buf));
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

void HandleIrcMessage(int sck, IrcMessage *msg)
{
    /*
        switch () {

        }
    */
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

int SendSslIrcMsg(SSL *ssl, const char *fmt, ...)
{
    char buf[512];
    va_list ap;

    va_start(ap, fmt);
    int nprinted = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    ssize_t total_sent = 0;
    while (total_sent < nprinted)
    {
        ssize_t nsent = SSL_write(ssl, buf, nprinted);
        if (nsent <= 0)
        {
            if (errno == EINTR)
                continue;
            else
                return 0;
        }

        total_sent += nsent;
    }

    return 1;
}

int SendIrcMsg(int fd, const char *fmt, ...)
{
    char buf[512];
    va_list ap;

    va_start(ap, fmt);
    int nprinted = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    ssize_t total_sent = 0;
    while (total_sent < nprinted)
    {
        ssize_t nsent = send(fd, buf, nprinted, 0);
        if (nsent <= 0)
        {
            if (errno == EINTR)
                continue;
            else
                return 0;
        }

        total_sent += nsent;
    }

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
    close(sck);
done:
    if (listp != NULL)
        freeaddrinfo(listp);
    return sck;
}

SSL *InitSslOnSocket(int sckfd)
{
    SSL_library_init();
    SSL_load_error_strings();

    SSL_CTX *ctx = SSL_CTX_new(TLS_method());
    if (ctx == NULL)
    {
        printf("Failed to create SSL context\n");
        return NULL;
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL); // for now...!
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);

    if (!SSL_CTX_set_default_verify_paths(ctx))
    {
        printf("Failed to set default ssl verify paths\n");
        return NULL;
    }

    SSL *ssl = SSL_new(ctx);
    if (ssl == NULL)
    {
        printf("Failed to create SSL\n");
        return NULL;
    }

    SSL_set_fd(ssl, sckfd);

    if (SSL_connect(ssl) == -1)
    {
        printf("Failed to SSL_connect()\n");
        return NULL;
    }

    return ssl;
}
