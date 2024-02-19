/*
middleware:
spawns 8 child processes 
each child process:
    wait for incoming client connection 
    on recv():
        spawn a new thread to handle client
        implementation of TOB using FIFO queue and LC
        send response to client 
        exit thread
parent exits
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

#define PORT 4096
#define BASEPORT 10000 // start port number for child servers
#define NCHILD 8 // number of child servers
#define BACKLOG 8
#define MAXCLIENTSIZE 1024 // maximum

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

int main(int argc, char *argv[]) {
    int num_server = NCHILD;
    if (argc > 1) {
        num_server = atoi(argv[1]);
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

    // configure child servers
    for(int i=0; i<num_server; i++) {
        int pid;
        pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) { // child process
            int port_num = BASEPORT + i;
            char port[6]; // asssume port numbers < 65536
            sprintf(port, "%d", port_num);
            char recv_buffer[MAXCLIENTSIZE];
            int sock_fd, client_fd;
            char client_ip[INET6_ADDRSTRLEN]; // IPv4(32 bit) or v6(128 bit)
            struct addrinfo *results, *ptr; // struct addrinfo holds socket address information
            struct sockaddr_storage client_addr; // storage for incoming client address IPv4/IPv6
            socklen_t sin_size; // size of client socket address IPv4/v6
            int status;

            // set up server socket addrinfo structs
            struct addrinfo hints; // set for server socket
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_UNSPEC; // IPv4/v6
            hints.ai_flags = AI_PASSIVE; // localhost
            hints.ai_socktype = SOCK_STREAM; // TCP

            // getaddrinfo returns pointer to list of addrinfo structs  
            // localhost
            if ((status = getaddrinfo(NULL, port, &hints, &results) != 0)) {
                fprintf(stderr, "server: getaddrinfo: %s\n", gai_strerror(status));
                return -1;
            }
            ptr = results; // list of addrinfo structs
            //bind to the first possible of addrinfo structs
            while(ptr->ai_next != NULL) {
                sock_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
                if (sock_fd == -1) {
                    perror("server: socket");
                    continue;
                }
                // socket in use error on server restart
                int y = 1;
                int opt_status;
                opt_status = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
                if (opt_status == -1) {
                    close(sock_fd);
                    fprintf(stderr, "port in use on server restart\n");
                    exit(1);
                }
                // bind error
                int bind_status;
                bind_status = bind(sock_fd, ptr->ai_addr, ptr->ai_addrlen);
                if (bind_status == -1 ) {
                    close(sock_fd);
                    perror("server:bind");
                    // continue;
                    exit(1);
                }
                break;
            }
            freeaddrinfo(results); // free results list
            if (ptr==NULL) {
                fprintf(stderr, "server: failed to bind to any\n");
                exit(1);
            }
            //listen for incoming connections
            int listen_status = listen(sock_fd, BACKLOG);
            if (listen_status == -1) {
                perror("listen");
                exit(1);
            }
            printf("[%d] server[%s]: waiting...\n", getpid(), port);
            // main accept loop
            while(1) {
                sin_size = sizeof(client_addr);
                client_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &sin_size);
                if (client_fd==-1) {
                    perror("recv");
                    continue;
                }
                // convert from n to p
                struct sockaddr *sock_addr;
                sock_addr = (struct sockaddr *)&client_addr;
                inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), client_ip, sizeof(client_ip));
                printf("[%d] server[%s]: connected from %s\n", getpid(), port, client_ip);

                // code to handle connection in a child process

                close(client_fd);
                exit(0);
            }

        } 
        else { //parent process
            printf("spawned child server on port %d [%d]\n", BASEPORT + i, pid);
        }

    }

    return 0;

}