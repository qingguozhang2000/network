#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <signal.h>

#define MAXPENDING 5 /* Maximum outstanding connection requests */
#define BUFFER_SIZE 1024

void DieWithError(char *errorMessage); /* Error handling function */
void HandleTCPClient(int clntSocket);  /* TCP client handling function */

int main(int argc, char *argv[])
{
    int servSock;                    /*Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned short echoServPort;     /* Server port */
    unsigned int clntLen;            /* Length of client address data structure */
    char *httpRequest;

    if (argc != 2) /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage: %s <Server Port>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]); /* First arg: local port */

    // printf("Server Port: %d\n", echoServPort);

    /* Create socket for incoming connections */
    if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    /* Bind to the local address */
    if (bind(servSock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");

    /* Mark the socket so it will listen for incoming connections */
    if (listen(servSock, MAXPENDING) < 0)
        DieWithError("listen() failed");

    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        clntLen = sizeof(echoClntAddr); /* Wait for a client to connect */
        if ((clntSock = accept(servSock, (struct sockaddr *)&echoClntAddr, &clntLen)) < 0)
            DieWithError("accept() failed");
        /* clntSock is connected to a client! */
        printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));
        HandleTCPClient(clntSock);

        close(clntSock);

        // For Ctrl C
        signal(SIGINT, SIG_DFL);
    }
}

void DieWithError(char *errorMessage)
{
    puts(errorMessage);
}

void HandleTCPClient(int clntSocket) /* TCP client handling function */
{
    char httpRequest[1000];
    // if HTTP request is good
    recv(clntSocket, httpRequest, 1000, 0);
    printf("HTTP Request: %s\n", httpRequest);
    // WHAT separates an HTTP request/response from just text?
    char buffer[1024] = "HTTP/1.1 200 OK\r\n";
    send(clntSocket, buffer, strlen(buffer), 0);
    // if there is a file request:
    // Find the file
    // If file doesn't exist, send error 404
    // handle bad HTTP request
    // Close connection and wait for the next one
    close(clntSocket);
}
/* NOT REACHED */
