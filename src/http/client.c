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
#define RESPONSE_BUF_SIZE 1000000

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
int connect_http_server(const struct HttpServerInfo *p_server_info);
void fill_get_request(struct HttpServerInfo *p_server_info, char *p_request);
void send_http_request(int sock, const char *p_request);

void start_timer(struct Timer *timer);
void stop_timer(struct Timer *timer);
void report_time(struct Timer *timer, char *message);

int main(int argc, char *argv[])
{
    struct Timer timer;
    start_timer(&timer);

    struct HttpServerInfo server_info = parse(argc, argv);
    int sock = connect_http_server(&server_info);

    char request[REQUEST_BUF_SIZE];
    fill_get_request(&server_info, request);
    send_http_request(sock, request);

    close(sock);

    stop_timer(&timer);
    report_time(&timer, "The whole request");

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

int connect_http_server(const struct HttpServerInfo *p_server_info)
{
    struct Timer timer;
    start_timer(&timer);

    int sock; /* Socket descriptor */

    // Set up our criteria for what socket we want
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;

    // Get a list of potential sockets
    struct addrinfo *p_addr_info;
    int rc = getaddrinfo(p_server_info->host, p_server_info->port, &hints, &p_addr_info);
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

    stop_timer(&timer);
    report_time(&timer, "Connecting to server");

    return sock;
}

void fill_get_request(struct HttpServerInfo *p_server_info, char *p_request)
{
    sprintf(p_request, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", p_server_info->path, p_server_info->host);
    int request_length = strlen(p_request);

    printf("%s", p_request);
    printf("Request Length: %d\n", request_length);
}

void send_http_request(int sock, const char *p_request)
{
    struct Timer timer;
    start_timer(&timer);

    int request_length = strlen(p_request);
    int rc = send(sock, p_request, request_length, 0);
    if (rc == -1)
    {
        perror("Send request failed");
    }

    // Receive response
    char response[RESPONSE_BUF_SIZE];
    memset(response, 0, RESPONSE_BUF_SIZE);
    int total_ength = 0;
    int length = 0;
    while ((length = recv(sock, response, RESPONSE_BUF_SIZE, MSG_WAITALL)) > 0)
    {
        printf("%s", response);
        total_ength += length;
    }

    printf("Response length: %d\n", total_ength);
    printf("\n"); /* Print a final linefeed */

    stop_timer(&timer);
    report_time(&timer, "Send request and receive response");
}

void start_timer(struct Timer *timer)
{
    timer->begin_time = clock();
}

void stop_timer(struct Timer *timer)
{
    timer->end_time = clock();
}

void report_time(struct Timer *timer, char *message)
{
    printf("%s took: %d ms\n", message, timer->end_time - timer->begin_time);
}
