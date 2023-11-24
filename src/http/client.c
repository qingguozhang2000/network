#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <ctype.h>
// #include <find.hpp>

#define RCVBUFSIZE 32                  /* Size of receive buffer */

void DieWithError(char *errorMessage); /* Error handling function */
void checkError(int rc);

int main(int argc, char *argv[])
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */
    unsigned short servPort;         /* Echo server port */
    char *server_port;             /* Allows us to enter the server port number*/
    char *servIP;                  /* Server IP address (dotted quad) */
    int time_measure_mode;
    // int slash_index;
    char *URL; /* Entered URL */
    char *domain;
    char *subdirectory;
    int bytesRcvd, totalBytesRcvd; /* Bytes read in single recv()
    and total bytes read */
    int rc;

    if (argc == 2)
    {
        // printf("args: 2\n");
        URL = argv[1];
        server_port = "80";
    }
    else if (argc == 3)
    {
        // printf("args: 3\n");
        // Ex: ./http_client server_url port_number
        URL = argv[1];
        server_port = argv[2];
        time_measure_mode = 0;
        ;
    }
    else if (argc == 4)
    {
        // printf("args: 4\n");
        // Ex: ./http_client [-options] server_url port_number
        URL = argv[2];
        server_port = argv[3];
        time_measure_mode = 1;
    }
    else /*if ((argc > 1) || (argc < 5))*/ /* Test for correct number of arguments */
    {
        // printf("args: N/A\n");
        fprintf(stderr, "Usage: %s <Server IP> <Echo Word> [<Echo Port>]\n",
                argv[0]);
        exit(1);
    }

    // Parse our URL into domain and subdirectory
    if (subdirectory = strstr(URL, "/"))
    {
        domain = strndup(URL, subdirectory - URL);
    }
    else
    {
        domain = URL;
        subdirectory = "/";
    }

    // Set up our address info structs
    struct addrinfo hints;
    struct addrinfo *servinfo, *p; /*hints: what IPs we want, servinfo: linked list of IP addresses*/

    int rv;

    // Set up our criteria for what socket we want
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;

    // Get a list of potential sockets
    if ((rv = getaddrinfo(domain, server_port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    int old_time = clock();
    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        // If we can't make the socket
        if ((sock = socket(p->ai_family, p->ai_socktype,
                           p->ai_protocol)) == -1)
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

        // Convert number to IP address:
        struct in_addr addr = {p->ai_addr};

        break; // if we get here, we must have connected successfully
    }

    if (p == NULL)
    {
        // looped off the end of the list with no connection
        fprintf(stderr, "failed to connect\n");
        exit(2);
    }

    // Set up our request stuff
    char request[1000];
    // char requestHostAddress[] = URL;

    // Make our request
    sprintf(request, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", subdirectory, domain);

    printf("%s", request);

    int actual_length = strlen(request);
    // Send request
    // IS this an actual HTTP request? I didn't have to use the IP address, I didn't have to request any file
    // sendto(server_port, request, 1000, 0, p, actual_length);
    rc = send(server_port, request, actual_length, 0);
    checkError(rc);
    printf("Request Length: %d\n", actual_length);
    // If we requested timer:
    if (argc == 4)
    {
        int new_time = clock();
        int elapsed_time = new_time - old_time;
        printf("RTT: %d\n", elapsed_time);
    }

    // Set up our response stuff
    int response_size = 1000;
    char response[1000];

    // Receive response
    rc = recv(server_port, response, response_size, 0);
    checkError(rc);
    printf("Response length: %d\n", rc);
    printf("Response: %s\n", response);

    freeaddrinfo(servinfo); // all done with this structure

    printf("\n"); /* Print a final linefeed */
    close(sock);
    exit(0);
}

void DieWithError(char *errorMessage)
{
    printf(errorMessage);
    printf("\n");
}

void checkError(int rc)
{
    if (rc == -1)
    {
        printf("Error Number: %d\n", errno);
    }
}
