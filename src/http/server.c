#include <stdio.h>      /* for printf() and fprintf() */
#include <stdbool.h>    /* for using true false */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <signal.h>

#define MAXPENDING 5 // Maximum outstanding connection requests
#define BUFFER_SIZE 1000000

struct RequestInfo
{
    char version[5]; // HTTP/1.1 etc.
    char method[10]; // GET, POST, PUT, DEELETE etc.
    char path[100];
    char body[1000];
};

int parse_server_port(int argc, char *argv[]);
int open_server_socket(int server_port);
void respond_to_client_request(int server_sock);
void process_client_request(int client_sock);
void parse_http_request(char *request, struct RequestInfo *request_info);
void send_response(int client_sock, struct RequestInfo *request_info);

int main(int argc, char *argv[])
{
    int server_port = parse_server_port(argc, argv);
    int server_sock = open_server_socket(server_port);

    while (true)
    {
        respond_to_client_request(server_sock);
        signal(SIGINT, SIG_DFL); // this allows one to stop server with Ctrl-C
    }

    close(server_sock);
}

int parse_server_port(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <Server Port>\n", argv[0]);
        exit(1);
    }

    return atoi(argv[1]);
}

int open_server_socket(int server_port)
{
    /* Create socket for incoming connections */
    int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock < 0)
    {
        perror("Opening server socket failed");
        exit(2);
    }

    // set up server address from server port
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));  // Zero out structure
    serverAddress.sin_family = AF_INET;                // Internet address family
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface
    serverAddress.sin_port = htons(server_port);       // Local port

    // bind server addess to server socket
    int rc = bind(server_sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (rc < 0)
    {
        perror("Faile to bind server addess to server socket");
        exit(3);
    }

    // have the server socket listen for incoming connections
    if (listen(server_sock, MAXPENDING) < 0)
    {
        perror("Server socket failed to listen for incoming connections");
        exit(4);
    }

    return server_sock;
}

void respond_to_client_request(int server_sock)
{
    struct sockaddr_in client_address;
    unsigned int length = sizeof(client_address);

    int client_sock = accept(server_sock, (struct sockaddr *)&client_address, &length);
    if (client_sock < 0)
    {
        perror("accept() failed");
        return;
    }

    // client_sock is connected to a client
    printf("Handling client %s\n", inet_ntoa(client_address.sin_addr));
    process_client_request(client_sock);

    // Close connection
    close(client_sock);
}

void process_client_request(int client_sock)
{
    char request[BUFFER_SIZE];
    struct RequestInfo request_info;

    recv(client_sock, request, BUFFER_SIZE, 0);
    printf("HTTP Request: %s\n", request);

    parse_http_request(request, &request_info);

    send_response(client_sock, &request_info);
}

void parse_http_request(char *request, struct RequestInfo *request_info)
{
    // we will only look at the 1st buffer
    sscanf(request, "%s /%s HTTP/%s",
           request_info->method,
           request_info->path,
           request_info->version);
}

void send_response(int client_sock, struct RequestInfo *request_info)
{
    if (strcmp(request_info->method, "GET") != 0)
    {
        char *response = "HTTP/1.1 405 Method Not Allowed\r\n";
        send(client_sock, response, strlen(response), 0);
        return;
    }

    // path is assumed to be the file name
    FILE *fp = fopen(request_info->path, "r");
    if (fp == NULL)
    {
        perror("File not found");
        char *response = "HTTP/1.1 404 Not Found\r\n";
        send(client_sock, response, strlen(response), 0);
        return;
    }

    char response[BUFFER_SIZE] = "HTTP/1.1 200 OK\r\n";
    char buffer[1000];
    while (fgets(buffer, 1000, fp) != NULL)
    {
        strncat(response, buffer, strlen(buffer));
    }

    send(client_sock, response, strlen(response), 0);

    fclose(fp);
}
