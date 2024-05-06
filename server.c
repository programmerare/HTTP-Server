#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define MAX_REQUEST_SIZE 2000

int server_fd;

void parse_request(char *request){
    regex_t regex;
    regcomp(&regex, "^GET /([^ ]*) HTTP/1", REG_EXTENDED);

    if(regexec(&regex, request, 0, NULL, 0) == 0){
        printf("Determined GET request!\n");
    }
}

void handle_request(int client_fd){
    char request[MAX_REQUEST_SIZE];

    ssize_t request_size = recv(client_fd, request, MAX_REQUEST_SIZE, 0);
    printf("%s\n", request);
}

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

        shutdown(client_fd, SHUT_RDWR);
    }

    return 0;
}