#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Placeholder for the IP address of the sender, assuming localhost for simplicity
#define SENDER_IP "127.0.0.1"

void send_p2p_ack(int sender_port, int base_pid);

void send_p2p_ack(int sender_port, int base_pid) {
    int sockfd;
    struct sockaddr_in servaddr;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(sender_port);
    servaddr.sin_addr.s_addr = inet_addr(SENDER_IP);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        printf("Connection with the server failed...\n");
        perror("connect");
        close(sockfd);
        return;
    }

    // Prepare the ACK message
    char ack_msg[1024];
    snprintf(ack_msg, sizeof(ack_msg), "ACK from PID %d", getpid());

    // Send the ACK message
    send(sockfd, ack_msg, strlen(ack_msg), 0);

    printf("ACK sent to PID %d at port %d\n", base_pid, sender_port);

    // Close the socket
    close(sockfd);
}
