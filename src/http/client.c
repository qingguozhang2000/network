#include <stdio.h>  /* for printf() and fprintf() */
#include <stdlib.h> /* for atoi() and exit() */
#include <unistd.h> /* for close() */
#include <string.h> /* for memset() */
#include <time.h>
#include <ctype.h>
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h> /* for sockaddr_in and inet_addr() */

#define REQUEST_BUF_SIZE 1000
#define RESPONSE_BUF_SIZE 1000

struct HttpServerInfo
{
    char *host;
    char *port;
    char *path;
};

struct Timer {
    int begin_time;
    int end_time;
};


struct HttpServerInfo parse(int argc, char *argv[]);
int connectHttpServer(struct HttpServerInfo *server);
void fillGetRequest(struct HttpServerInfo *server, char *request);
void sendHttpRequest(int sock, char *request);

void startTimer(struct Timer *timer);
void stopTimer(struct Timer *timer);
void reportTime(struct Timer *timer, char *message);

int main(int argc, char *argv[])
{
    struct Timer timer;
    startTimer(&timer);

    struct HttpServerInfo server = parse(argc, argv);
    int sock = connectHttpServer(&server);

    char request[REQUEST_BUF_SIZE];
    fillGetRequest(&server, request);
    sendHttpRequest(sock, request);

    close(sock);

    stopTimer(&timer);
    reportTime(&timer, "The whole request");

    exit(0);
}

struct HttpServerInfo parse(int argc, char *argv[])
{
    struct HttpServerInfo server;

    char *url = argc >= 2 ? argv[1] : NULL;
    char *port = argc >= 3 ? argv[2] : "80";

    if (!url)
    {
        fprintf(stderr, "Usage: %s <host> [<port>]\n", argv[0]);
        exit(1);
    }

    // Parse our url into host and path
    char *path = strstr(url, "/");
    server.host = path ? strndup(url, path - url) : url;
    server.path = path ? path : "/";
    server.port = port;

    return server;
}

int connectHttpServer(struct HttpServerInfo *server)
{
    struct Timer timer;
    startTimer(&timer);

    int sock; /* Socket descriptor */

    // Set up our criteria for what socket we want
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;

    // Get a list of potential sockets
    struct addrinfo *p_addr_info;
    int rc = getaddrinfo(server->host, server->port, &hints, &p_addr_info);
    if (rc != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        exit(1);
    }

    // loop through all the results and connect to the first we can
    struct addrinfo *p;
    for (p = p_addr_info; p != NULL; p = p->ai_next)
    {
        // If we can't make the socket
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock == -1)
        {
            perror("socket");
            continue;
        }

        // If we can't connect to the socket
        if (connect(sock, p->ai_addr, p->ai_addrlen) == -1)
        {
            perror("connect");
            close(sock);
            continue;
        }

        // if we get here, we must have connected successfully
        break;
    }

    if (p == NULL)
    {
        // looped off the end of the list with no connection
        fprintf(stderr, "failed to connect\n");
        exit(2);
    }

    freeaddrinfo(p_addr_info); // all done with this structure

    stopTimer(&timer);
    reportTime(&timer, "Connecting to server");

    return sock;
}

void fillGetRequest(struct HttpServerInfo *server, char *request)
{
    sprintf(request, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", server->path, server->host);
    int request_length = strlen(request);

    printf("%s", request);
    printf("Request Length: %d\n", request_length);
}

void sendHttpRequest(int sock, char *request)
{
    struct Timer timer;
    startTimer(&timer);

    int request_length = strlen(request);
    int rc = send(sock, request, request_length, 0);
    if (rc == -1)
    {
        perror("Send request failed");
    }

    // Receive response
    char response[RESPONSE_BUF_SIZE];
    memset(response, 0, RESPONSE_BUF_SIZE);
    rc = recv(sock, response, RESPONSE_BUF_SIZE, 0);
    if (rc == -1)
    {
        perror("Receive response failed");
    }

    printf("Response length: %d\n", rc);
    printf("Response: %s\n", response);
    printf("\n"); /* Print a final linefeed */

    stopTimer(&timer);
    reportTime(&timer, "Send request and receive response");
}

void startTimer(struct Timer *timer)
{
    timer->begin_time = clock();
}

void stopTimer(struct Timer *timer)
{
    timer->end_time = clock();
}

void reportTime(struct Timer *timer, char *message)
{
    printf("%s took: %d ms\n", message, timer->end_time - timer->begin_time);
}
