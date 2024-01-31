/*
** multithreaded tcp server
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <pthread.h>
#include "threadqueue.h"

#define PORT "4096"
#define BACKLOG 200
#define HEADERSIZE 40
#define THREADPOOL 40

// thread pool
pthread_t t_pool[THREADPOOL];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;

void *set_value(void *args);
void *get_in_addr(struct sockaddr *sa);
char *parse_header(char *recv_header, int *actual_size_ptr);
int parse_body(int client_fd, char *buffer, int actual_size);
void *handle_client(void *client_fd_ptr);

void *set_value(void *args) {
    // waiting on condition var
    while(1) {
        int *client_sock;
        pthread_mutex_lock(&mutex);
        if ((client_sock = dequeue()) == NULL) { // check for empty queue
            pthread_cond_wait(&cond_var, &mutex); // suspend thread release lock
            client_sock = dequeue(); // connection recieved
        }
        pthread_mutex_unlock(&mutex);
        if (client_sock != NULL) {
            handle_client(client_sock); // client handler for the thread
        }
    }
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// parse header and malloc actual value size
char *parse_header(char *recv_header, int *actual_size_ptr) {
    char *token;
    token = strtok(recv_header, "\r\n");
    int value_size = atoi(token);
    token = strtok(NULL, "\r\n");
    int actual_size = atoi(token);
    token = strtok(NULL, "\r\n");
    char key[13];
    strncpy(key, token, 12);
    key[12] = '\0';
    int buffer_size = actual_size + 13 + 8;
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("recv: malloc");
        exit(1);
    }
    *actual_size_ptr = actual_size;
    return buffer;
}

// recv() until recv_bytes == actual value size
int parse_body(int client_fd, char *buffer, int actual_size) {
    int recv_bytes = 0;
    int recv_size;
    while(recv_bytes < actual_size) {
        recv_size = recv(client_fd, buffer+recv_bytes, actual_size-recv_bytes, 0);
        if (recv_size == -1) {
            perror("recv: value");
            free(buffer);
            close(client_fd);
            exit(1);
        }
        recv_bytes += recv_size;
    }
    if (recv_bytes != actual_size) {
        fprintf(stderr, "Partial Recieve\nRecv: %d  Actual: %d\n", recv_bytes, actual_size);
        free(buffer);
        close(client_fd);
        return -1;
    }
    buffer[recv_bytes] = '\0';
    return 0;
}

void *handle_client(void *client_fd_ptr) {
    int client_fd = *((int*)client_fd_ptr);
    free(client_fd_ptr);
    char recv_header[HEADERSIZE];
    int header_bytes;
    header_bytes = recv(client_fd, &recv_header, 34, 0); // read first 34 bytes header
    if (header_bytes == -1) {
        perror("recv:header_bytes");
        exit(1);
    }
    int actual_size;
    recv_header[header_bytes] = '\0';
    char *buffer = parse_header(recv_header, &actual_size);
    printf("header:\n%s\n", buffer);
    int rv;
    if ((rv = parse_body(client_fd, buffer, actual_size)) != 0) {
        perror("recv: body");
        exit(1);
    }
    printf("body:\n%s\n", buffer);
    printf("strlen:%zu\n", strlen(buffer));

    char *ack = "STORED\r\n";
    send(client_fd, ack, strlen(ack), 0);
    printf("ack sent\n");     
    return NULL;
}

// main
int main(void) {
    int sockfd, client_fd;
    struct addrinfo *results, *ptr;
    struct addrinfo hints;
    struct sockaddr_storage client_addr;
    socklen_t sin_size;
    int status;
    char client_ip[INET6_ADDRSTRLEN]; // IPv4(32 bit) or v6(128 bit)
    char recv_header[HEADERSIZE];

    // create threads
    for (int i = 0; i < THREADPOOL; i++) {
        pthread_create(&t_pool[i], NULL, set_value, NULL);
    }

    // set up addrinfo structs
    memset(&hints, 0, sizeof(hints)); // set memory block to 0
    hints.ai_family = AF_UNSPEC; // IPv4/v6
    hints.ai_flags = AI_PASSIVE; // localhost
    hints.ai_socktype = SOCK_STREAM; // TCP
    if ((status = getaddrinfo(NULL, PORT, &hints, &results) != 0)) {
        fprintf(stderr, "getaddrinfo: %s", gai_strerror(status));
        return -1;
    }
    // loop through linked list of addrinfo and bind to the first possible
    ptr = results;
    while(ptr->ai_next != NULL) {
        sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        // error check 
        // socket creation
        if (sockfd == -1) {
            perror("server:socket");
            continue;
        }
        // socket in use error on server restart
        int y = 1;
        int opt_status;
        opt_status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
        if (opt_status == -1) {
            perror("sockopt");
            exit(1);
        }
        // bind error
        int bind_status;
        bind_status = bind(sockfd, ptr->ai_addr, ptr->ai_addrlen);
        if (bind_status == -1 ) {
            close(sockfd);
            perror("server:bind");
            continue;
        }
        break;
    }

    freeaddrinfo(results); // free addrinfo list
    if (ptr == NULL) {
        fprintf(stderr, "server:failed to bind\n"); // failed to bind to any
        exit(1);
    }
    int listen_status = listen(sockfd, BACKLOG); // listen to incoming client
    if (listen_status == -1) {
        perror("listen");
        exit(1);
    }

    //main accept loop
    while(1) {
        printf("server: waiting...\n");
        sin_size = sizeof(client_addr);
        client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        // convert from network to presentation
        struct sockaddr *sock_addr;
        sock_addr = (struct sockaddr *)&client_addr;
        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), client_ip, sizeof(client_ip));
        printf("server: connected from %s\n", client_ip);

        int *client_ptr = malloc(sizeof(int));
        *client_ptr = client_fd;
        // enqueue client fd - lock for shared queue
        pthread_mutex_lock(&mutex);
        enqueue(client_ptr);
        pthread_cond_signal(&cond_var); // signal for incoming connection
        pthread_mutex_unlock(&mutex);
    }

    return 0;
}