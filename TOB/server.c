/*
Implementation of TOB using LC:
Middleware:
    - n number of single threaded socket processes 
    - each process listens on BASEPORT upto the number of processes
    - each process maintains a FIFO queue and local timestamps (event counters for send/recv)
    - port selection for an incoming request is handled on the client end
    - the main process synchronoulsy waits for each child to exit
Events for each process:
    - send/recv message from clients
    - send/recv message from processes
Application:
    - a shared file for printing messages from process
Testing:
    - per process log file
    - each event(send/recv) is logged with the queue state and local timestamp
    - testing with single client concurrent message ordering
    - testing with multiple clients concurrent message ordering
Potential Improvements
    - async waiting by parent process?
    - shared resource access using broadcast messages
*/

#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BASEPORT 10000 // base port for the first process
#define NUMPROC 10 // default number of processes
#define BACKLOG 8 // number of backlog connections to listen
#define MAXMSGSIZE 4098 // maximum number of message bytes recv from client

void *get_in_addr(struct sockaddr *sa);
void sock_serve(int port_num);

// get sockaddr IPv4/v6
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// sock_serve sets up socket listening on input port
// it accepts the incoming connection and stores the client fd on a heap
// calls function for queue management and broadcast
void sock_serve(int port_num) {
    char port[6]; // assume port < 65536
    int s = snprintf(port, sizeof(port), "%d", port_num);
    char recv_buffer[MAXMSGSIZE];
    int sockstatus;

    // set up socket config
    int sock_fd, client_fd; // scoket fd
    char client_ip[INET6_ADDRSTRLEN]; // to store client incoming ip address (32 or 128 bits) (IPv4/v6)
    struct addrinfo *results, *ptr; // pointer for getaddrinfo() results
    struct sockaddr_storage client_addr; // to store client address 
    socklen_t sin_size; // store addrlen size
    
    // set up socket addrinfo structs
    struct addrinfo hints; // struct with config for socket
    memset(&hints, 0, sizeof(hints)); // flush
    hints.ai_family = AF_UNSPEC; // IPv4/v6
    hints.ai_flags = AI_PASSIVE; // localhost ip
    hints.ai_socktype = SOCK_STREAM; // TCP 

    sockstatus = getaddrinfo(NULL, port, &hints, &results);
    if (sockstatus != 0) {
        perror("getaddrinfo");
        exit(1);
    }
    // loop and create for the first possible
    ptr = results; 
    while (ptr->ai_next != NULL) {
        sock_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sock_fd == -1) {
            perror("server:socket");
            exit(1);
        }
        // socket in use error on server restart
        int y = 1;
        int opt_status;
        opt_status = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
        if (opt_status == -1) {
            perror("sockopt");
            exit(1);
        }
        // bind socket
        int bind_status;
        bind_status = bind(sock_fd, ptr->ai_addr, ptr->ai_addrlen);
        if (bind_status != 0) {
            perror("server:bind");
            continue;
        }
        break;
    }
    freeaddrinfo(results); // free returned list
    int listen_status = listen(sock_fd, BACKLOG);
    if (listen_status == -1) {
        perror("server:listen");
        exit(1);
    }
    printf("[%d] server[%s]: waiting...\n", getpid(), port);
    // main accept loop
    while (1) {
        sin_size = sizeof(client_addr);
        client_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &sin_size);
        if (client_fd == -1) {
            perror("server:accept");
            continue;
        }
        // print incoming ip
        // convert from netw to prsntation
        struct sockaddr *sock_addr;
        sock_addr = (struct sockaddr *)&client_addr;
        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), client_ip, sizeof(client_ip));
        printf("[%d] server[%s]: connected from %s\n", getpid(), port, client_ip);

        int *client_fd_cpy = malloc(sizeof(int));
        *client_fd_cpy = client_fd;
        // implement algorithm for queue management and broadcast and application 

        close(client_fd);
        free(client_fd_cpy);

    }

}


int main(int argc, char *argv[]) {
    int num_process = NUMPROC;
    if (argc == 2) num_process = atoi(argv[1]);
    for (int i=0;i < num_process; i++) {
        pid_t child_pid;
        child_pid = fork();
        if (child_pid == -1) {
            perror("fork");
            exit(1);
        }
        if (child_pid == 0) {
            //child process
            int port_num = BASEPORT + i;
            sock_serve(port_num);
            exit(0);
        } 
        // else {
        //     //parent
        //     // printf("spawned child server [PID: %d]\n", child_pid);
        // }
    }
    // parent
    // int status;
    pid_t wait_pid;
    while((wait_pid = waitpid(-1, NULL, 0)) > 0) {}; // sync wait for all child to exit. block the calling process
    // printf("wait over!\n");

    return 0;
}