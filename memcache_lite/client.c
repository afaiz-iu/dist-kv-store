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

char* encode_msg(const char* key, const char* value, int value_size, int actual_size, char* msg_buffer) {
    /*
    ** message format
    ** 8 Bytes for input value_size + 2 \r\n 
    ** 8 Bytes for actual value size + 2 \r\n 
    ** 12 Bytes for input key string + 2 \r\n 
    ** actual value_size Bytes for value string + 4 \r\n\r\n
    */    
    int offset = 0;
    offset += sprintf(msg_buffer, "%-8d\r\n", value_size); // user value size
    offset += sprintf(msg_buffer+offset, "%-8d\r\n", actual_size); // value_size
    offset += sprintf(msg_buffer+offset, "%-12s\r\n", key); // key 
    memcpy(msg_buffer + offset, value, actual_size);
    offset += actual_size;

    msg_buffer[offset++] = '\r';
    msg_buffer[offset++] = '\n';
    msg_buffer[offset++] = '\r';
    msg_buffer[offset++] = '\n';
    msg_buffer[offset] = '\0';
    return msg_buffer;
}

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
    int actual_size = strlen(value);
    if (value_size < (actual_size + 10)) {
        fprintf(stderr, "usage: %s Insufficient value bytes passed\n", argv[1]);
        return 1;
    }
    int buffer_size = 8 + 2 + 8 + 2 + 12 + 2 + actual_size + 8 + 4;
    char *msg_buffer = malloc(buffer_size);
    if (msg_buffer == NULL) {
        perror("message buffer");
        exit(1);
    }
    memset(msg_buffer, 0, buffer_size);
    encode_msg(key, value, value_size, actual_size, msg_buffer);
    printf("msg:\n%s", msg_buffer);
    printf("buffer size:%d\n", buffer_size);
    sendFullBuffer(sockfd, msg_buffer, &buffer_size);
    free(msg_buffer);

    // recieve from host
    recv_bytes = recv(sockfd, recv_buffer, MAXRECVBYTES-1, 0);
    printf("recv bytes: %d\n", recv_bytes);
    if (recv_bytes == -1) {
        perror("client: recv");
        exit(1);
    }
    if (recv_bytes == 0) {
        fprintf(stderr, "%d: connection closed by host", recv_bytes);
        return 1;
    }
    recv_buffer[recv_bytes] = '\0'; // add null 
    printf("client: recieved %s", recv_buffer);

    return 0;
}