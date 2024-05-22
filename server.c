#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define PORT 8080
#define MAX_REQUEST_SIZE 2000
#define RESPONSE_HEAD_SIZE 200
#define RESPONSE_CONTENT_SIZE 2000
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
        printf("Determined GET request for ");
        // extract url from the request
        if(pmatch[1].rm_so != -1 && pmatch[1].rm_eo != -1) {
            int start = pmatch[1].rm_so - 1;
            int end = pmatch[1].rm_eo;
            snprintf(url, MAX_URL_SIZE, "%.*s", end - start, request + start);
        }
    }

    printf("'%s'\n", url);

    regfree(&regex);
}

/*
    parse_file_extension
    ---------------------
    Description:
    Returns the file extension of a given filename excluding '.'

    Parameter:
    char **dest: The destination where to the file extension will be stored
    char *filename: A string containing the filename

    Return:
    void: This function does not return a value
*/
void parse_file_extension(char **dest, char *filename){
    *dest = strrchr(filename, '.') + 1;
}

/*
    send_response
    ---------------------
    Description:
    Reads in the content of the requested file, sends a response header and a response body containing the files content.
    If the file does not exist a response header with 404 error code is send to the client.

    Parameters:
    int client_fd: An integer working as a file descriptor. It is an identifier for the client's socket
    char *filename: A string holding the name of the file to be opened

    return:
    void: This function does not return a value
*/
void send_response(int client_fd, char *path){
    char response_header[RESPONSE_HEAD_SIZE];
    char response_content[RESPONSE_CONTENT_SIZE];

    FILE *fp = fopen(path, "r");
    size_t file_size;
    size_t read_bytes = 0;

    if(fp == NULL){
        printf("Requested file not found!\n");
        snprintf(response_header, RESPONSE_HEAD_SIZE, "HTTP/1.1 404 not found\r\nContent-Type: text/html\r\nContent-Length:%d\r\n\r\n", 0);
        send(client_fd, response_header, RESPONSE_HEAD_SIZE, 0);
        return;
    }

    // read in file content
    read_bytes = fread(response_content, 1, RESPONSE_CONTENT_SIZE, fp);
    response_content[read_bytes] = '\0';
    fclose(fp);

    // retrieve file extension
    char *file_extension;
    parse_file_extension(&file_extension, path);

    if((strcmp(file_extension, "html")) == 0){
        // send response header
        snprintf(response_header, RESPONSE_HEAD_SIZE, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %zu\r\n\r\n", read_bytes);
        send(client_fd, response_header, strlen(response_header), 0);
    }
    else if((strcmp(file_extension, "css")) == 0){
        // send response header
        snprintf(response_header, RESPONSE_HEAD_SIZE, "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\nContent-Length: %zu\r\n\r\n", read_bytes);
        send(client_fd, response_header, strlen(response_header), 0);
    }

    // send response content
    send(client_fd, response_content, read_bytes, 0);
}

/*
    handle_request
    ---------------------
    Description:
    This function recieves an http request using the recv function from the sys/socket library and reads it into a buffer.
    It then calls the parse_request function on the request and retrieves the extracted url.
    Depending on the url it will call the send_response function.

    Parameter:
    void *arg: A void pointer pointing to the clients file descriptor. It is an identifier for the client's socket.

    Return:
    void: This function does not return a value.
*/
void handle_request(void *arg){
    int client_fd = *((int*)arg);

    char request[MAX_REQUEST_SIZE];
    char url[MAX_URL_SIZE];

    ssize_t request_size = recv(client_fd, request, MAX_REQUEST_SIZE, 0);
    parse_request(request, url);

    if((strcmp(url, "/")) == 0){
        send_response(client_fd, "views/index.html");
    }
    else if((strcmp(url, "/contact")) == 0){
        send_response(client_fd, "views/contact.html");
    }
    else if((strcmp(url, "/styles.css")) == 0){
        send_response(client_fd, "public/css/styles.css");
    }
    else{
        send_response(client_fd, "404");
    }

    printf("Shutting down client socket!\n");
    shutdown(client_fd, SHUT_RDWR);
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
    if(listen(server_fd, 10) == -1){
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
        printf("User connected!\n");

        // handle the client request
        pthread_t thread;
        pthread_create(&thread, NULL, (void*)handle_request, (void*)&client_fd);
        pthread_detach(thread);
    }

    return 0;
}