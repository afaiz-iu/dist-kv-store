/*
** server.c - concurrent tcp server with child process handling incoming request
*/

/*
** system calls
** sockaddr_in -> parallel structure to sockaddr struct for IPv4
** struct sockaddr_in {
**     short int sin_family; // AF_INET
**     unsigned short int sin_port; 
**     struct in_addr sin_addr; // address
**     unsigned char sin_zero[8]; // same size as struct sockaddr
** };

** POINTER TO sockaddr_in CAN BE CAST TO sockaddr AND VICE VERSA

** getaddrinfo() reurns a linked list of struct addrinfo
** struct addrinfo {
**     int ai_flags;
**     int ai_family;
**     int ai_socktype;
**     int ai_protocol;
**     struct sockaddr *ai_addr;
**     char *ai_canonname;
**     struct addrinfo *next; //linked list next node
** }

** int getaddrinfo(const char *node, // IP
** const char *service, // port
** const struct addrinfo *hints, //pointer to addrinfo
** struct addrinfo **res //pointer to linked list of addrinfo nodes
** );

** int socket(int domain, int type, int protocol); 

** int bind(int sockfd, struct sockaddr *my_addr, int addrlen);
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

#define PORT "4096"
#define BACKLOG 8
#define MAXVALUESIZE 4096

// sigchild action to reap child process
void sigaction_sigchld(int p) {
    // save and restore the errno since waitpid can overwrite 
    int s_err = errno;

    // wait() for any child
    // parent is not blocked
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = s_err;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int sendFullBuffer(int sfd, char *buf, int *len) {
    int total = 0;
    int bleft = *len;
    int t;
    while (total < *len) {
        // add partial send bytes to pointer
        t = send(sfd, buf+total, bleft, 0);
        if (t == -1) {break;}
        total += t;
        bleft -= t;
    }
    *len = total; // update number actually sent
    return t == -1?1:0; // return 1 on failure 
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
        // failed to bind to any
        fprintf(stderr, "server:failed to bind\n");
        exit(1);
    }
    
    // listen to incoming client
    int listen_status = listen(sockfd, BACKLOG);
    if (listen_status == -1) {
        perror("listen");
        exit(1);
    }

    // set up sigchild handler
    // reap all child zombies
    struct sigaction sigact;
    sigact.sa_handler = sigaction_sigchld;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sigact, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting...\n");
    char recv_buffer[MAXVALUESIZE];

    //main accept loop
    while(1) {
        sin_size = sizeof(client_addr);
        client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }
        // convert from network to preentation
        struct sockaddr *sock_addr;
        sock_addr = (struct sockaddr *)&client_addr;
        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), client_ip, sizeof(client_ip));
        printf("server: connected from %s\n", client_ip);

        // code to recieve from client in a subprocess
        // fork and pass client handler in child process
        char r1[5];
        char r2[2048];
        int nb1, nb2;
        if(!fork()) {
            close(sockfd); //close sock fd in child
            nb1 = recv(client_fd, &r1, 4, 0);
            if (nb1 == -1) {
                perror("recv:nb1");
                exit(1);
            }
            r1[nb1] = '\0';
            printf("r1:\n%s\n", r1);
            nb2 = recv(client_fd, &r2, 2047, 0);
            if (nb2 == -1) {
                perror("recv:nb1");
                exit(1);
            }
            r2[nb2] = '\0';
            printf("r2:\n%s\n", r2);
            // int numbytes;
            // numbytes = recv(client_fd, &recv_buffer, MAXVALUESIZE-1, 0);
            // if (numbytes == -1) {
            //     perror("recv");
            //     exit(1);
            // }
            // recv_buffer[numbytes] = '\0';
            // printf("recived msg:\n%s", recv_buffer);

            printf("sending ack..\n");
            char *ack = "STORED\r\n";
            send(client_fd, ack, strlen(ack), 0);
            printf("ack sent\n");
            close(client_fd);
            exit(0);
        }
        close(client_fd);
    }

    return 0;
}