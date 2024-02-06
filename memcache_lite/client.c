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

int sendFullBuffer(int sfd, char *buf, int *len);
char* encode_set(const char* key, const char* value, size_t key_size, size_t value_size, size_t actual_size, char* msg_buffer);
char* encode_get(const char* key, size_t key_size, char* msg_buffer);

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

char* encode_get(const char* key, size_t key_size, char* msg_buffer) {
    /*
    ** message format
    ** 6 Bytes for key length size \r\n
    ** key string \r\n 
    */    
    int offset = 0;
    offset += sprintf(msg_buffer+offset, "%s\r\n", "get"); // operation
    // offset += sprintf(msg_buffer+offset, "%-6zu\r\n", key_size); // key length
    offset += sprintf(msg_buffer+offset, "%s\r\n", key); // key 
    msg_buffer[offset] = '\0';
    return msg_buffer;
}

char* encode_set(const char* key, const char* value, size_t key_size, size_t value_size, size_t actual_size, char* msg_buffer) {
    /*
    ** message format
    ** 8 Bytes for input value_size \r\n 
    ** 8 Bytes for actual value size \r\n 
    ** 6 Bytes for key length size \r\n
    ** key string \r\n 
    ** value string \r\n\r\n
    */    
    int offset = 0;
    offset += sprintf(msg_buffer+offset, "%s\r\n", "set"); // operation
    offset += sprintf(msg_buffer+offset, "%-8zu\r\n", value_size); // user value size
    offset += sprintf(msg_buffer+offset, "%-8zu\r\n", actual_size); // value_size
    offset += sprintf(msg_buffer+offset, "%-6zu\r\n", key_size); // key length
    offset += sprintf(msg_buffer+offset, "%s\r\n", key); // key 
    memcpy(msg_buffer + offset, value, actual_size);
    offset += actual_size;

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

    if ((argc !=3) && (argc != 5)) {
        fprintf(stderr, "usage: %s <get/set> <key> [<value-size-bytes> <value>]\n", argv[0]);
        return 1;
    }
    char *operation = argv[1];
    if (strcmp(operation, "set") != 0 && strcmp(operation, "get") != 0) {
        fprintf(stderr, "Invalid operation: %s\n", operation);
        return 1;
    }
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
        if (sockfd == -1) {
            perror("client:socket");
            continue;
        }
        // connect to localhost
        if (connect(sockfd, ptr->ai_addr, ptr->ai_addrlen) == -1) {
            close(sockfd);
            perror("client:connect");
            exit(1);
        }
        break;
    }
    if (ptr == NULL) {
        fprintf(stderr, "client: failed to connect\n"); // failed to connect
        return 1;
    }
    freeaddrinfo(results);

    if (strcmp(operation, "get") == 0) {
        // Get operation
        if (argc != 3) {
            fprintf(stderr, "usage: %s get <key>\n", argv[0]);
            return 1;
        }
        char *key = argv[2];
        size_t key_size = strlen(key);
        int buffer_size = 5+6+2;
        char *msg_buffer = malloc(buffer_size);
        if (msg_buffer == NULL) {
            perror("message buffer");
            exit(1);
        }
        memset(msg_buffer, 0, buffer_size);
        encode_get(key, key_size, msg_buffer);
        sendFullBuffer(sockfd, msg_buffer, &buffer_size);
        free(msg_buffer);
    } 
    if (strcmp(operation, "set") == 0) {
        // Set operation
        if (argc != 5) {
            fprintf(stderr, "usage: %s set <key> <value-size-bytes> <value>\n", argv[0]);
            return 1;
        }
        char *key = argv[2];
        size_t key_size = strlen(key);
        size_t value_size = strtoul(argv[3], NULL, 10);
        if (value_size == 0 || errno == ERANGE) {
            fprintf(stderr, "usage: %s Invalid value size (must be a positive integer)\n", argv[0]);
            return 1;
        }
        char *value = argv[4];
        size_t actual_size = strlen(value);
        if (value_size < (actual_size + 2)) {
            fprintf(stderr, "usage: %s Insufficient value bytes passed\n", argv[1]);
            return 1;
        }
        // 5 bytes for operation; 8 bytes for value_size; 8 bytes for input_size; 6 bytes for key length
        int buffer_size = 5 + 8 + 2 + 8 + 2 + 6 + 2 + actual_size + 8;
        char *msg_buffer = malloc(buffer_size);
        if (msg_buffer == NULL) {
            perror("message buffer");
            exit(1);
        }
        memset(msg_buffer, 0, buffer_size);
        encode_set(key, value, key_size, value_size, actual_size, msg_buffer);
        sendFullBuffer(sockfd, msg_buffer, &buffer_size);
        free(msg_buffer);
    }

    // recieve ack
    recv_bytes = recv(sockfd, recv_buffer, MAXRECVBYTES-1, 0);
    if (recv_bytes == -1) {
        perror("client: recv");
        exit(1);
    }
    if (recv_bytes == 0) {
        fprintf(stderr, "%d: connection closed by host", recv_bytes);
        return 1;
    }
    recv_buffer[recv_bytes] = '\0'; // add null 
    printf("client: recieved..\n%s", recv_buffer);

    return 0;
}