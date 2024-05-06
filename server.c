#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define MAX_REQUEST_SIZE 2000
#define MAX_URL_SIZE 100

int server_fd;

/*
    parse_request
    ---------------------
    Description:
    The parse_request function parses an http-request and extracts it's url using regex

    Parameter:
    char *request: A pointer to a string containing an http-request
    char *url: A pointer to a string where the extracted url will be stored

    Return:
    void: The function does not return a value
*/
void parse_request(char *request, char *url){
    regex_t regex;
    regcomp(&regex, "^GET /([^ ]*) HTTP/1", REG_EXTENDED);

    size_t nmatch = 2;
    regmatch_t pmatch[2];


    if(regexec(&regex, request, nmatch, pmatch, 0) == 0){
        printf("Determined GET request!\n");
        // extract url from the request
        if(pmatch[1].rm_so != -1 && pmatch[1].rm_eo != -1) {
            int start = pmatch[1].rm_so - 1;
            int end = pmatch[1].rm_eo;
            snprintf(url, MAX_URL_SIZE, "%.*s", end - start, request + start);
        }
    }

    printf("%s\n", url);

    regfree(&regex);
}

/*
    handle_request
    ---------------------
    Description:
    This function recieves an http request using the recv function from the sys/socket library and reads it into a buffer.
    It then calls the parse_request function on the request and retrieves the extracted url.

    Parameter:
    int client_fd: An integer working as a file descriptor. It is an identifier for the client's socket

    Return:
    void: This function does not return a value
*/
void handle_request(int client_fd){
    char request[MAX_REQUEST_SIZE];
    char url[MAX_URL_SIZE];

    ssize_t request_size = recv(client_fd, request, MAX_REQUEST_SIZE, 0);
    printf("%s\n", request);
    parse_request(request, url);
}

/*
    server_shutdown
    ---------------------
    Description:
    This function shuts down the server's socket by calling the shutdown function
    of the sys/socket library on the server file descriptor.
    It than exits the programin using exit(0);

    Parameter:
    No parametesr. The function is called on two signals SIGTERM and SIGINT defined in the signal library

    Return:
    void: This function does not return a value
*/
void server_shutdown(){
    printf("\nShutting down server...\n");
    shutdown(server_fd, SHUT_RDWR);
    exit(0);
}

int main(int argc, char **argv){

    // signal handler for termination
    signal(SIGTERM, server_shutdown);
    signal(SIGINT, server_shutdown);

    // create a socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // specify address and port
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // bind address and port to socket
    if(bind(server_fd, (struct sockaddr*)&server_addr, (socklen_t)sizeof(server_addr)) == -1){
        fprintf(stderr, "Error binding address and port to socket!\n");
        server_shutdown();
    }

    // listen for connections
    printf("Listening for connections...\n");
    if(listen(server_fd, 1) == -1){
        fprintf(stderr, "Error listening for connections!\n");
        server_shutdown();
    }

    // accept connections
    while(1){
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);

        if(client_fd == -1){
            fprintf(stderr, "Error accepting connection!\n");
            server_shutdown();
            break;
        }
        printf("Accepted connection!\n");

        // handle the client request
        handle_request(client_fd);

        printf("Shutting down client socket!\n");
        shutdown(client_fd, SHUT_RDWR);
    }

    return 0;
}