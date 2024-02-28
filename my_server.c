/*
** EPITECH PROJECT, 2023
** my_ftp
** File description:
** my_server.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <limits.h>
#include <stdbool.h>

#define MAX_CLIENTS 5

void *handle_client(void *client_socket);
void start_server(int port, const char* directory);

void *handle_client(void *client_socket) 
{

    int sock = *((int*)client_socket);
    char buffer[1024] = {0};
    int valread;
    bool anonymous = false;
    bool password = false;

    char *message = "220 Service ready for new user.\n";
    write(sock, message, strlen(message));
    //send(sock, message, strlen(message), 0);

    while((valread = read(sock, buffer, 1024)) > 0) {
        
        if (strncmp(buffer, "QUIT", 4) == 0) {
            char *quitMsg = "221 Service closing control connection.\n";
            write(sock, quitMsg, strlen(quitMsg));
            //send(sock, quitMsg, strlen(quitMsg), 0);
            break;
        }
        if (strncmp(buffer, "LIST", 4) == 0) {
            printf("[FTP Server] User entered LIST command.\n");
            char *listMsg = "221 Service listing directories.\n";
            write(sock,listMsg , strlen(listMsg));
            //send(sock, listMsg, strlen(listMsg), 0);
        }
        if (strncmp(buffer, "USER Anonymous", strlen("USER Anonymous")) == 0 && anonymous == false) {
            printf("User Anonymous ready to log in.\n");
            anonymous = true;
            char *listMsg = "331 User Anonymous okay, need password.\n";
            write(sock,listMsg , strlen(listMsg));
            //send(sock, listMsg, strlen(listMsg), 0);
        }

        if (strncmp(buffer, "PASS ", strlen("PASS ")) == 0 && anonymous == true) {
            printf("User logged in.\n");
            password = true;
            char *listMsg = "230 User logged in, proceed.\n";
            write(sock, listMsg , strlen(listMsg));
            //send(sock, listMsg, strlen(listMsg), 0);
        }
        if (strncmp(buffer, "CWD ", 4) == 0 && anonymous == true && password == true) {

            char path[1024];

            sscanf(buffer + 4, "%s", path);
            
            if (chdir(path) == 0) {
                char *msg = "250 Directory successfully changed.\n";
                write(sock, msg, strlen(msg));
            } else {
                char *errMsg = "550 Failed to change directory.\n";
                write(sock, errMsg, strlen(errMsg));
            }
        }

        if (strncmp(buffer, "DELE ", 5) == 0 && anonymous == true && password == true) {

            char filename[1024];

            sscanf(buffer + 5, "%s", filename);
            
            if (remove(filename) == 0) {
                char *msg = "250 File deleted successfully.\n";
                write(sock, msg, strlen(msg));
            } else {
                char *errMsg = "550 File deletion failed.\n";
                write(sock, errMsg, strlen(errMsg));
            }
        }

        if (strncmp(buffer, "PWD", 3) == 0 && anonymous == true && password == true) {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                char msg[1064];
                sprintf(msg, "257 \"%s\" is the current directory.\n", cwd);
                write(sock, msg, strlen(msg));
            } else {
                char *errMsg = "550 Failed to get current directory.\n";
                write(sock, errMsg, strlen(errMsg));
            }
        }

        memset(buffer, 0, 1024);
    }

    printf("[FTP Server] Client disconnected.\n");
    close(sock);
    return NULL;
}

void start_server(int port, const char* directory) 
{
    int server_fd, *new_socket_ptr;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("[FTP Server] Listening on port %d\n", port);

    while(1) {
        new_socket_ptr = malloc(sizeof(int));
        if (new_socket_ptr == NULL) {
            perror("Failed to allocate memory for new socket");
            continue;
        }

        if ((*new_socket_ptr = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("accept");
            free(new_socket_ptr); 
            exit(EXIT_FAILURE);
        }

        printf("[FTP Server] Accept new Client.\n");

        pthread_t thread_id;
        if(pthread_create(&thread_id, NULL, handle_client, (void*)new_socket_ptr) != 0) {
            perror("pthread_create failed");
            free(new_socket_ptr); 
        }
        pthread_detach(thread_id);
    }
}

int main(int ac, char **av) 
{
    if (ac != 3) {
        printf("Usage: %s PORT path\n", av[0]);
        return 1;
    }

    int port = atoi(av[1]);
    char realPath[PATH_MAX];
    const char* directory = av[2];

    if (realpath(av[2], realPath) == NULL) {
        perror("realpath failed");
        return 1;
    }

    if (chdir(directory) != 0) {
        perror("chdir failed");
        return 1;
    }

    printf("[FTP Server] Starting server.\n");
    printf("[FTP Server] Port: [%d] - Path Default: [%s]\n", port, realPath);

    start_server(port, directory);

    return 0;
}
