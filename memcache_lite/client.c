/*
** tcp client
** accept key value input pair 
** send to server
** recieve ack
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
#define MAXRECVBYTES 40

int main(int argc, char *argv[]) {
    struct addrinfo hints, *results, *ptr;
    int sockfd, status, recv_bytes;
    socklen_t sin_size;
    char recv_buffer[MAXRECVBYTES];

    // set up structs
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((status = getaddrinfo("localhost", PORT, &hints, &results)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    // loop through returned lnkdlst and set up first available
    ptr = results;
    while(ptr->ai_next != NULL) {
        // attempt socket create
        sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        //error check
        //socket creation
        if (sockfd == -1) {
            perror("client:socket");
            continue;
        }
        // connect to localhost
        if (connect(sockfd, ptr->ai_addr, ptr->ai_addrlen) == -1) {
            close(sockfd);
            perror("client:connect");
            continue;
        }
        break;
    }

    if (ptr == NULL) {
        // failed to connect
        fprintf(stderr, "client: failed to connect\n");
        return 1;
    }

    freeaddrinfo(results);

    if (argc != 4) {
        fprintf(stderr, "usage: %s <key> <value-size-bytes> <value>\n", argv[0]);
        return 1;
    }
    char *key = argv[1];
    int value_size = atoi(argv[2]);
    char *value = argv[3];
    if (value_size < (strlen(value) + 20)) {
        fprintf(stderr, "usage: %s Insufficient value bytes passed\n", argv[1]);
        return 1;
    }
    size_t buffer_size;
    buffer_size = value_size + strlen(key) + 20; // 20 extra bytes for headers, newline, null
    char *msg_buffer = malloc(buffer_size);
    // construct the message
    // set <key> <value-size-bytes> \r\n
    // <value> \r\n 
    sprintf(msg_buffer, "set %s %d\r\n%s\r\n", key, value_size, value);
    printf("msg:\n%s", msg_buffer);
    send(sockfd, msg_buffer, buffer_size, 0); // send message
    free(msg_buffer);

    // recieve from host
    recv_bytes = recv(sockfd, recv_buffer, MAXRECVBYTES, 0);
    printf("recv bytes: %d\n", recv_bytes);
    if (recv_bytes == -1) {
        perror("client: recv");
        exit(1);
    }
    if (recv_bytes == 0) {
        fprintf(stderr, "%d: connection closed by host", recv_bytes);
        return 1;
    }
    recv_buffer[recv_bytes] = '\0'; // add eol 
    printf("client: recieved %s", recv_buffer);

    return 0;
}