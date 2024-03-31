/*
IPC message format:
    - <PID>\r<OPERATION>\r<LC>\r<MSG_ID>\r\n (20 bytes with padding)

process queue:
    - node data items: LC timestamp, msg_id, msg_size, *msg_buffer
    - next: pointer to next node

Totally Ordered Multicast Algorithm : 2 Rounds
- All messages timestamped with sender’s logical time
- Sender sends to all recipients, including itself
- When a message is received:
    - 1. It is put into a local queue
    - 2. Queue is ordered based on timestamp
    - 3. Send ack to original sender (no broadcast) if msg in head of queue
- Sender marks message ‘ready’ when it is head of queue and all acks rcvd
- Sender broadcasts second round of ‘ready’ messages to others.
- Message is delivered to application only when:
    - 1. It is at the head of the queue
    - 2. ‘Ready’ message has been received.
*/

/*
- recv from application -> issue (1)
- broadcast (2)
- recv from self (3,2) -> (3)
- parse msg type -> ack/event
    - enqueue node
    - sort queue
    - check if head of queue
    - send ack to sender (4)
    - recv ack from self (5)
    - recv ack from all (6+)
    - broadcast ready (x)
    - send to application
    - recv from application
*/

/*
parse header to determine the operation
separate execution path basis the parsed header value
*/

#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "queue.h"
#include "client_handle.h"

#define ISSUE "ISSUE"
#define BROAD "BROAD"
#define SEND "SEND0"
#define ACK "ACK00"
#define READY "READY"
#define MAXMSGSIZE 2048
#define BASEPORT 10000 // base port for the first process
#define ACK_IP "127.0.0.1" // ip address for p2p ack in 2 round TOM

extern pthread_mutex_t file_mutex;
extern node_t *head;
extern node_t *tail;
extern FILE* shared_file;

// function to generate uid by using bitwise operation 
// (pid << 12) OR (LC) -> uid
unsigned int generate_uid(unsigned int lc) {
    unsigned int pid = getpid();
    unsigned int uid = pid << 12 | lc;  // left shift by 12 OR lc counter
    return uid;
}

// send until buffer empty
int send_buffer(int *pid_fd_ptr, char *buffer, size_t *buf_len) {
    int pid_fd = *pid_fd_ptr;
    int total = 0;
    int bleft = *buf_len;
    int bsent = 0;
    while (total < *buf_len) {
        bsent = send(pid_fd, buffer+bsent, bleft, 0);
        if (bsent == -1) break;
        total += bsent;
        bleft -= bsent;
    }
    *buf_len = total; // update actually sent
    // printf("inside buffer sent(sent bytes):\t%d\n", bsent);
    return bsent == -1?1:0; // return 1 on send failure
}

// recv until buffer empty
int recv_buffer(int *pid_fd_ptr, char *buffer, int *buf_len) {
    int pid_fd = *pid_fd_ptr;
    int total = 0;
    int bleft = *buf_len;
    int brecv = 0;
    while (total < *buf_len && bleft > 0) {
        brecv = recv(pid_fd, buffer+brecv, bleft, 0);
        if (brecv == -1) {break;}
        else if (brecv == 0) {break;} // connection closed
        total += brecv;
        bleft -= brecv;
    }
    *buf_len = total; // update actual recv bytes
    // printf("inside recv_buffer():\t%d\n", total);
    return brecv == -1?1:0; // return 1 on recv failure    
}

void check_and_ack_head() {
    printf("[%d] inside check ack\n", getpid());
    print_queue(getpid());
    order_queue(); // sort
    if (head != NULL && head->ack_flg == 0) {
        head->ack_flg = 1;
        send_p2p_ack(head->port_num, head->pid, head->uid); 
        printf("[%d] ack called\n", getpid());
    }
}

// send ack message for the message UID and ack process id
// setup a tcp p2p connection with the issuing process
void send_p2p_ack(int sender_port, int base_pid, unsigned int uid) {
    int sockfd;
    struct sockaddr_in servaddr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr)); // flush

    // set up socket structs
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(sender_port);
    servaddr.sin_addr.s_addr = inet_addr(ACK_IP);

    // connect
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd); // free
        return;
    }

    // encode message
    // Format: "ACK00\r\nPID:<pid> UID:<uid>"
    char ack_msg[1024];
    snprintf(ack_msg, sizeof(ack_msg), "ACK00\r\nPID:%d UID:%u", base_pid, uid);

    // send
    if (send(sockfd, ack_msg, strlen(ack_msg), 0) < 0) {
        perror("send");
    } else {
        printf("ACK sent to PID %d at port %d for UID %u\n", base_pid, sender_port, uid);
    }
    close(sockfd); // free socket
}

void write_shared(const char* buffer, size_t buf_len) {
    // lock()
    pthread_mutex_lock(&file_mutex);
    if (shared_file == NULL) {
        fprintf(stderr, "shared file not init\n");
        pthread_mutex_unlock(&file_mutex); // release
        return;
    }

    // write buffer to shared
    fwrite(buffer, sizeof(char), buf_len, shared_file);
    fflush(shared_file); // flush 

    pthread_mutex_unlock(&file_mutex); // release
}

// handler for ack recieved on the issuing process
// increments ack_count; checks if ready can be sent by comparing ack_count to nprocs
// on all ack recieved calls write_shared() -> acquire lock; write to shared file the node buffer; release lock
void handle_ack(int *client_fd_ptr, unsigned int *lc) {
    int client_fd = *client_fd_ptr;
    char ack_msg[1024];
    int msg_bytes;

    if ((msg_bytes = recv(client_fd, ack_msg, sizeof(ack_msg), 0)) < 0) {
        perror("server: recv");
        return;
    }
    ack_msg[msg_bytes] = '\0'; 
    printf("recieved ack msg: %s\n", ack_msg);

    int received_pid;
    unsigned int received_uid;
    if (sscanf(ack_msg, "PID:%d UID:%u", &received_pid, &received_uid) == 2) {
        // printf("Received ACK for PID %d and UID %u\n", received_pid, received_uid);
        node_t* node = find_node(received_uid);
        if (node != NULL) {
            node->ack_count += 1;
            if (node->ack_count == node->nprocs) {
                // all ACK received, mark message as ready and broadcast
                char *buffer = node->buffer;
                size_t buf_len = node->buf_len;
                // buffer[buf_len++] = "\n";
                buffer[buf_len] = '\0';
                broadcast_ready_message(node, lc);
                // code to write the node buffer to a shared file
                // acquire the mutex and write to file
                write_shared(buffer, buf_len);
            }
        } else {
            printf("Node with UID %u not found.\n", received_uid);
        }
    } else {
        perror("Failed to parse ACK message");
    }
}

void handle_ready(int *client_fd_ptr) {
    int client_fd = *client_fd_ptr;
    char ready_msg[1024];
    int msg_bytes;

    if ((msg_bytes = recv(client_fd, ready_msg, sizeof(ready_msg), 0)) < 0) {
        perror("server: recv");
        return;
    }
    ready_msg[msg_bytes] = '\0'; 
    unsigned int received_uid;
    if (sscanf(ready_msg, "UID:%u", &received_uid) == 1) {
        printf("Received READY for UID %u\n", received_uid);
        dequeue(received_uid); // dequeue node with the received uid
    } else {
        perror("Failed to parse READY");
    }
    printf("[%d] queue in handle_ready()", getpid());
    print_queue(getpid());
}



// encode the ipc broadcast msg
// header -> BROAD\r\n (7B)
// payload -> 40B + msg_len
// payload -> <uid(12 bytes zero-padded)><pid(8 bytes zero-padded)> <port_num(8 bytes zero-padded)> <LC timestamp(4 bytes zero-padded)> <message length< (6 bytes zero-padded)><message>\r\n
void encode_broadcast_buffer(node_t *node, char *buffer, unsigned int *lc) {
    pid_t base_pid = node->pid;
    int base_port = node->port_num;
    size_t buffer_len = node->buf_len;
    unsigned int uid = node->uid; 
    
    const char *header = "BROAD\r\n"; // header
    // copy header
    memcpy(buffer, header, strlen(header));
    int offset = strlen(header);

    // encode uid (12 byte zero-padded)
    char uid_str[13]; // 12 bytes for uid + 1 byte for null terminator
    snprintf(uid_str, sizeof(uid_str), "%12u", uid);
    memcpy(buffer + offset, uid_str, 12);
    offset += 12;

    // encode base pid (8 byte zero-padded)
    char pid_str[9]; // 8 bytes for pid + 1 byte for null terminator
    snprintf(pid_str, sizeof(pid_str), "%08d", base_pid);
    memcpy(buffer + offset, pid_str, 8);
    offset += 8;
    
    // encode base port num (8 byte zero-padded)
    char port_str[9]; // 8 bytes for port + 1 byte for null terminator
    snprintf(port_str, sizeof(port_str), "%08d", base_port);
    memcpy(buffer + offset, port_str, 8);
    offset += 8;    

    // encode Lamport clock timestamp (4 byte zero-padded)
    char lc_str[5]; // 4 bytes for Lamport clock + 1 byte for null terminator
    snprintf(lc_str, sizeof(lc_str), "%04u", *lc); // use the passed timestamp
    memcpy(buffer + offset, lc_str, 4);
    offset += 4;

    // encode message length value buffer_len (6 byte zero-padded)
    char len_str[7]; // 6 bytes for message length + 1 byte for null terminator
    snprintf(len_str, sizeof(len_str), "%06zu", buffer_len); // 
    memcpy(buffer + offset, len_str, 6);
    offset += 6;

    // encode actual message
    memcpy(buffer + offset, node->buffer, buffer_len);
    offset += buffer_len;

    // add \r\n at the end
    memcpy(buffer + offset, "\r\n", 2);
    offset += 2;
    buffer[offset] = '\0';
}

int total_broadcast(node_t *node, unsigned int *lc) {
    *lc += 1; // increment timestamp once for bradcast operation(send all)

    //send from BASEPORT until nprocs
    int nprocs = node->nprocs;
    pid_t base_pid = node->pid;
    int base_port = node->port_num;
    int *client_fds = malloc(sizeof(int) * nprocs); // array of client fds on heap
    if (client_fds == NULL) {
        perror("malloc");
        return 1;
    }

    // open connections to all processes, including itself
    for (int i = 0; i < nprocs; i++) {
        client_fds[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (client_fds[i] == -1) {
            perror("socket");
            free(client_fds);
            return 1;
        }
        // set up sockaddr struct for each mid socket
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(BASEPORT + i); // current port -> BASEPORT + [i]
        server_addr.sin_addr.s_addr = INADDR_ANY;
        // connect to middleware socket
        if (connect(client_fds[i], (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            perror("connect");
            close(client_fds[i]); // close socket on error
            free(client_fds); // free
            return 1;
        }
    }
    // header -> BROAD\r\n (7B)
    // payload -> 40B + msg_len
    size_t buffer_len = node->buf_len; 
    size_t total_buffer_size = 40 + buffer_len; 
    char *buffer = malloc(total_buffer_size); // allocate buffer
    if (buffer == NULL) {
        perror("malloc");
        // close_sockets(client_fds, nprocs); // Close sockets on error
        free(client_fds);
        return 1;
    }

    encode_broadcast_buffer(node, buffer, lc); // encode ipc payload

    // printf("lc: %u\nbraodacast msg:  %s\n", *lc, buffer);
    printf("[%d] base uid<%u> sending broadcast... <size: %zu> \n", getpid(), node->uid, total_buffer_size);
    // send formatted message to other middleware socket fds
    for (int i = 0; i < nprocs; i++) {
        int send_stat = send_buffer(&client_fds[i], buffer, &total_buffer_size);
        if (send_stat != 0) {
            perror("send_buffer");
            for (int j = 0; j < nprocs; j++) {
                close(client_fds[j]);
            }
            free(client_fds);
            free(buffer);
            return 1;
        }
    }
    // printf("all broadcast sent\n");

    free(buffer);
    free(client_fds);

    return 0;

}

void broadcast_ready_message(node_t *node, unsigned int *lc) {
    *lc += 1; // increment timestamp once for broadcast operation (send all)

    int nprocs = node->nprocs;
    int *client_fds = malloc(sizeof(int) * nprocs); // client fds on heap
    if (client_fds == NULL) {
        perror("malloc");
        return;
    }

    // open conn to all, including itself
    for (int i = 0; i < nprocs; i++) {
        client_fds[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (client_fds[i] == -1) {
            perror("socket");
            free(client_fds);
            return;
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(BASEPORT + i); // Current port -> BASEPORT + [i]
        server_addr.sin_addr.s_addr = INADDR_ANY;

        if (connect(client_fds[i], (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            perror("connect");
            close(client_fds[i]); // close
            free(client_fds); // free
            return;
        }
    }

    // encode ready message
    char ready_msg[1024];
    snprintf(ready_msg, sizeof(ready_msg), "READY\r\nUID:%u", node->uid);

    // send "READY" message to other middleware socket fds
    for (int i = 0; i < nprocs; i++) {
        if (send(client_fds[i], ready_msg, strlen(ready_msg), 0) < 0) {
            perror("send");
        }
    }

    printf("READY message sent for UID %u\n", node->uid);

    // Close all sockets and free allocated memory
    for (int j = 0; j < nprocs; j++) {
        close(client_fds[j]);
    }
    free(client_fds);
}


void handle_issue(int *client_fd_ptr, int num_process, unsigned int *lc, int base_port) {
    *lc += 1; // increment lc for ISSUE 

    // read buffer size
    // read buffer, broadcast msg
    int client_fd = *client_fd_ptr;
    char prefix_payload[7];
    int prefix_b;
    if ((prefix_b = recv(client_fd, &prefix_payload, sizeof(prefix_payload), 0)) <0) {
        perror("server: recv");
        return;
    }
    // *lc += 1; // recv -> increment lc
    prefix_payload[prefix_b - 1] = '\0'; // overwrite delimiter
    int msg_size = atoi(prefix_payload);
    char *buffer = malloc(msg_size + 1); // allocate heap
    if (buffer == NULL) {
        perror("malloc");
        return;
    }
    int issue_stat;
    int buf_len = msg_size;
    int issue_b;
    if (recv_buffer(client_fd_ptr, buffer, &buf_len) != 0) {
        perror("recv: issue");
        return;
    }
    // create node for the issued msg
    if (buffer[buf_len - 1] != '\0') {
        buffer[buf_len] = '\0'; 
        buf_len++;
    }
    pid_t pid_self = getpid();
    unsigned int lc_ = *lc; // copy lc
    unsigned int uid = generate_uid(lc_);
    int ack_flg = 0; // set ack to 0
    int ack_count = 0; // set ack count to 0
    node_t *node = create_node(pid_self, base_port, ack_flg, ack_count, buffer, buf_len, lc_, uid, num_process); // node created for ISSUE message
    // printf("[%d] node created:\nbuffer:%s\n", getpid(), node->buffer);

    total_broadcast(node, lc);  // call broadcast
    free(buffer);

}

void handle_broadcast(int *client_fd_ptr, int num_process, unsigned int *lc) {
    *lc += 1; // increment LC for broadcast recv()

    // "BROAD\r\n<uid(12 bytes zero-padded)><pid(8 bytes zero-padded)><port_num(8 bytes zero-padded)><LC timestamp(4 bytes zero-padded)><message length(6 bytes zero-padded)><message>\r\n"
    unsigned int lc_cpy = *lc;
    
    // buffer to hold the pid, LC timestamp, and message length
    int meta_len = 38; // 12 bytes for UID, 8 bytes for PID, 8 bytes for port, 4 for LC, 6 for message length
    char *metadata = malloc(meta_len + 1); // +1 for null terminator
    if (metadata == NULL) {
        perror("malloc");
        return;
    }

    // read metadata
    if (recv_buffer(client_fd_ptr, metadata, &meta_len) != 0) {
        perror("server: recv metadata");
        free(metadata);
        return;
    }
    metadata[meta_len] = '\0';

    // extract UID
    char uid_str[13];
    memcpy(uid_str, metadata, 12);
    uid_str[12] = '\0';
    char *endptr_uid;
    unsigned int uid = (unsigned int)strtoul(uid_str, &endptr_uid, 10); // Base 10

    // extract PID
    char pid_str[9];
    memcpy(pid_str, metadata+12, 8);
    pid_str[8] = '\0';
    int base_pid = atoi(pid_str);

    // extract port num
    char port_str[9];
    memcpy(port_str, metadata+20, 8);
    port_str[8] = '\0';
    int base_port = atoi(port_str);

    // extract Lamport clock timestamp
    char lc_str[5];
    memcpy(lc_str, metadata + 28, 4);
    lc_str[4] = '\0';
    char *endptr_lc;
    unsigned int node_lc = (unsigned int)strtoul(lc_str, &endptr_lc, 10); // Base 10

    // extract message length
    char len_str[7];
    memcpy(len_str, metadata + 32, 6);
    len_str[6] = '\0';
    int msg_len = atoi(len_str);

    // read actual message (last 5 bytes not being read;could not solve this issue)
    int client_fd = *client_fd_ptr;
    char message[msg_len+1];
    int msg_bytes;
    if ((msg_bytes = recv(client_fd, &message, sizeof(message), 0)) <0) {
        perror("server: recv");
        return;
    }
    message[msg_len] = '\0';
    printf("[%d] Received msg(%d): %s\n", getpid(), msg_bytes, message);

    
    // max(node_lc, lc_cpy)
    if (node_lc > lc_cpy) {
        *lc = node_lc; // ADJUST local process timestamp
        lc_cpy = *lc;
    }
    // create node for broadcast recv() and insert it into the queue
    // set ack_flg and ack_count to 0 on recv()
    int ack_flg = 0;
    int ack_count = 0;
    node_t *new_node = create_node(base_pid, base_port, ack_flg, ack_count, message, msg_len, node_lc, uid, num_process);
    printf("[%d] node created for the uid: %u\n", getpid(), new_node->uid);
    if (new_node != NULL) {
        enqueue(new_node);
        printf("[%d] message enqueued\n", getpid());
        // order_queue();
        print_queue(getpid());
        // on sending the ack send_p2p_ack() should set the ack_flg to 1
        check_and_ack_head();

    } else {
        printf("[%d] Failed to enqueue message.\n", getpid());
    }

    // Free the allocated message buffer
    // free(message);
}

void handle_client(int *client_fd_ptr, int port_num, int num_process, unsigned int *lc) {
    int *client_fd_cpy = malloc(sizeof(int));
    *client_fd_cpy = *client_fd_ptr;
    int client_fd = *client_fd_ptr;
    // unsigned int lc_ = *lc; // copy lc counter to local addr space
    // parse header
    char header[7];
    int head_b;
    if ((head_b = recv(client_fd, &header, sizeof(header), 0)) <0) {
        perror("server: recv");
        return;
    }
    // *lc += 1; // recv -> increment lc
    header[head_b] = '\0';
    size_t head_len = strlen(header);
    if (strncmp(header, ISSUE, 5) == 0) {
        handle_issue(client_fd_cpy, num_process, lc, port_num); // create node; issue broadcast
    }
    if (strncmp(header, BROAD, 5) == 0) {
        handle_broadcast(client_fd_cpy, num_process, lc); // parse ipc broadcast messages; call ack based on ack_flg 
    }
    if (strncmp(header, ACK, 5) == 0) {
        handle_ack(client_fd_cpy, lc);
    }
    if (strncmp(header, READY, 5) == 0) {
        handle_ready(client_fd_cpy);
    }
    char *ack = "ACK\r\n";
    send(*client_fd_cpy, ack, strlen(ack), 0);
    free(client_fd_cpy);
}

