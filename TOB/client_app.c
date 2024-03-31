/*
** tcp client
** accept string input from argv
** send to server
** recieve ack
** exit
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define PORTSFD "listening_ports.txt" // file to select a random listening port
#define MAXPORTS 10 // max number of ports to choose from
#define MAXRECVBYTES 40 // max recv bytes from server
// #define MAXMSGSIZE 2048 // max message size to be sent

int sendFullBuffer(int sfd, char *buf, size_t *len);
char* encode_set(const char* key, const char* value, size_t key_size, size_t value_size, size_t actual_size, char* msg_buffer);
char* encode_get(const char* key, size_t key_size, char* msg_buffer);

int sendFullBuffer(int sfd, char *buf, size_t *len) {
    int total = 0;
    size_t bleft = *len;
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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <message>\n", argv[0]);
        return 1;
    }

    struct addrinfo hints, *results, *ptr;
    int sockfd, status, num_ports, port_num, recv_bytes;
    char recv_buffer[MAXRECVBYTES];
    socklen_t sin_size;
    srand(time(NULL)); // seed for random number 
    // select a random port from listening ports
    int ports_fd = open(PORTSFD, O_RDONLY);
    if (ports_fd == -1) {
        perror("open");
        return 1;
    }
    char ports_buffer[MAXPORTS * 6]; // assuming port numbers less than 6 digits each
    ssize_t ports_b = read(ports_fd, ports_buffer, sizeof(ports_buffer));
    if (ports_b == -1) {
        perror("read");
        close(ports_fd);
        return 1;
    }
    close(ports_fd);
    num_ports = 0; // counter for # of ports read
    char *token = strtok(ports_buffer, "\n"); // tokenize on /n delim
    while (token != NULL) {
        num_ports++;
        token = strtok(NULL, "\n");
    }
    if (num_ports == 0) {
        fprintf(stderr, "Error: No ports found in file\n");
        return 1;
    }
    // choose a random port 
    port_num = 10000 + rand() % num_ports; // within range num_ports
    char port[6]; // assume port < 65536
    int s = snprintf(port, sizeof(port), "%d", port_num);
    printf("selected port: %s\n", port);

    // set up structs
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((status = getaddrinfo("localhost", port, &hints, &results)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }
    // loop through returned lnkdlst and set up first available
    ptr = results;
    while (ptr->ai_next != NULL) {
        // attempt socket create
        sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sockfd == -1) {
            perror("client:socket");
            continue;
        }
        // connect to server
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

    char *message = argv[1]; 
    size_t msg_len = strlen(message);
    if (msg_len > 0 && message[msg_len - 1] == '\n') {
        message[msg_len - 1] = '\0';  // remove newline and add null terminator
        msg_len--;
    }
    printf("msg len:%zu\n", msg_len);

    // allocate buffer for entire message
    // message format
    // header: ISSUE\r\n (total 7 bytes)
    // payload: <message length< (6 bytes zero-padded)>delimiter><actual message bytes>\r\n (total msg_len + 9 bytes)
    const char *delim = "|";
    size_t total_buffer_size = 7 + msg_len + 9; 
    char msg_buffer[total_buffer_size];

    // encode msg header
    const char *header = "ISSUE\r\n";
    memcpy(msg_buffer, header, strlen(header));

    // encode payload 
    int offset = strlen(header);
    if (snprintf(msg_buffer + offset, 7, "%06zu", msg_len) >= 7) {
        fprintf(stderr, "Error: Message length exceeds maximum size\n");
        close(sockfd);
        return 1;
    }

    offset += 6;
    memcpy(msg_buffer + offset, delim, strlen(delim));  // add delimiter
    offset += strlen(delim);
    memcpy(msg_buffer + offset, message, msg_len); // copy actual message to buffer
    offset += msg_len;
    msg_buffer[offset++] = '\r';
    msg_buffer[offset++] = '\n';
    msg_buffer[offset++] = '\0';

    total_buffer_size = offset; // update actual size of the buffer
    int send_stat = sendFullBuffer(sockfd, msg_buffer, &total_buffer_size);
    if (send_stat == 1) {
        perror("send");
        close(sockfd);
        return 1;
    }

    // recieve ack
    recv_bytes = recv(sockfd, recv_buffer, 3, 0);
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

    // close socket
    close(sockfd);

    return 0;
}