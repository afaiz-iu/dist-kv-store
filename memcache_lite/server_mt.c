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
#include <limits.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <pthread.h>
#include "threadqueue.h" // thread queue fifo functions
#include "hashmap.h" // in-memory hashmap functions

#define PORT "4096"  // port number for the application
#define BACKLOG 200  // number of backlog clients for listen()
#define HEADERSIZE 40 // max header size
#define CLIENTPOOL 40 // num threads to handle incoming client connect()
#define HASHPOOL 20 // num threads to handle set()/get() from hash map
#define HEADER_DELIM "\r\n" // delimiter used in message header 
#define KEYSIZE 12 // max key size 

// thread pool
pthread_t conn_pool[CLIENTPOOL];
pthread_t hash_pool[HASHPOOL];
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t client_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t hash_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t hash_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t hashmap_mutex = PTHREAD_MUTEX_INITIALIZER;

kv_node_t * hash_map[TABLESIZE];
FILE *log_file = NULL; // write-ahead log file for all set operations

void *client_threads(void *args);
void *get_in_addr(struct sockaddr *sa);
int parse_header_set(char *recv_header, size_t *actual_size, size_t *value_size, size_t *key_size);
char *parse_header_get(char *recv_header);
int parse_body(int client_fd, char *buffer, int actual_size);
void handle_client(void *client_fd_ptr);
void handle_hash(void *node);

// random number generator
float randnum(float smallNumber, float bigNumber)
{
    float diff = bigNumber - smallNumber;
    return (((float) rand() / RAND_MAX) * diff) + smallNumber;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void *client_threads(void *args) {
    // waiting on condition var
    while(1) {
        int *client_sock;
        pthread_mutex_lock(&client_mutex);
        if ((client_sock = dequeue()) == NULL) { // check for empty queue
            pthread_cond_wait(&client_cond, &client_mutex); // suspend thread release lock
            client_sock = dequeue(); // connection recieved
        }
        pthread_mutex_unlock(&client_mutex);
        if (client_sock != NULL) {
            handle_client(client_sock); // client handler for the thread
        }
    }
}

void *hash_threads(void *args) {
    //wait on condition var
    while(1) {
        // kv_node_t *tmp = malloc(sizeof(kv_node_t));
        node_hash_t *tmp;
        pthread_mutex_lock(&hash_mutex);
        if ((tmp = dequeue_hash()) == NULL) {  // check if queue empty
            pthread_cond_wait(&hash_cond, &hash_mutex); // suspend and release lock
            tmp = dequeue_hash(); // struct kv_node_t pointer
        }
        pthread_mutex_unlock(&hash_mutex); // release lock
        if (tmp != NULL) {
            handle_hash(tmp); // handler for hashing tmp
        }
    }
}

char *send_message_format(kv_node_t *node) {
    char *key_copy = strdup(node->key);
    char *value_copy = strdup(node->value);
    size_t user_size = node->user_size;
    size_t value_size = node->value_size;
    // 10 butes for storing user_size
    size_t buffer_size = strlen("VALUE ") + strlen(key_copy) + 1 + 10 + 1 + 2 + strlen(value_copy) + 2 + strlen("END\r\n") + 1;
    char *msg_buffer = malloc(buffer_size);
    int offset = snprintf(msg_buffer, buffer_size+1, "VALUE %s %zu", key_copy, user_size);
    memcpy(msg_buffer+offset, value_copy, value_size);
    offset += value_size;
    offset += snprintf(msg_buffer+offset, buffer_size-offset, "\r\nEND\r\n");
    msg_buffer[offset] = '\0';
    return msg_buffer;
}

void handle_hash(void *node) {
    node_hash_t *block = (node_hash_t*)node;
    kv_node_t *tmp = block->data;
    int client_fd = block->sock_fd;
    size_t val_size = tmp->value_size;
    size_t user_size = tmp->user_size;
    char *key_copy = strdup(tmp->key);
    unsigned char oper_flg = tmp->oper_flg;
    char *client_msg;
    if (oper_flg==0) {
        char *err_get = "Error: key not found\n";
        //get operation
        char *get_key = strdup(tmp->key);
        kv_node_t *tmp_node = malloc(sizeof(kv_node_t));        
        pthread_mutex_lock(&hashmap_mutex);
        tmp_node = get_node(get_key);
        pthread_mutex_unlock(&hashmap_mutex);
        if (tmp_node == NULL) {
            send(client_fd, err_get, strlen(err_get), 0);
            close(client_fd); // close client socket
            return;
        }
        client_msg = send_message_format(tmp_node);
        send(client_fd, client_msg, strlen(client_msg), 0);
        free(client_msg);
        close(client_fd); // close client socket
    } 
    if (oper_flg == 1) {
        char *err_set = "NOT-STORED\r\n";
        client_msg = strdup("STORED\r\n");
        bool set_stat;
        pthread_mutex_lock(&hashmap_mutex);
        fprintf(log_file, "SET %s %zu %zu %s\n", key_copy, user_size, val_size, tmp->value);
        fflush(log_file); // flush data
        set_stat=insert_node(tmp);
        pthread_mutex_unlock(&hashmap_mutex);
        if (set_stat != true) {
            send(client_fd, err_set, strlen(err_set), 0);
            close(client_fd); // close client socket
            return;
        }
        send(client_fd, client_msg, strlen(client_msg), 0);
        free(client_msg);
        close(client_fd); // close client socket
    }
}

char *parse_header_get(char *recv_header) {
    char *token, *saveptr;
    token = strtok_r(recv_header, HEADER_DELIM, &saveptr); // extract key
    if (token == NULL) {
        fprintf(stderr, "Error: Missing key in header\n");
        return NULL;
    }
    size_t key_len;
    key_len = strlen(token);
    char *get_key = strdup(token);
    return get_key;
}

// parse header
int parse_header_set(char *recv_header, size_t *actual_size, size_t *value_size, size_t *key_size) {
    char *token, *saveptr;
    token = strtok_r(recv_header, HEADER_DELIM, &saveptr); // extract value-size
    if (token == NULL) {
        fprintf(stderr, "Error: Missing value_size in header\n");
        return -1;
    }
    *value_size = strtoul(token, NULL, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "Error: Value size out of range\n");
        return -1;
    }
    token = strtok_r(NULL, HEADER_DELIM, &saveptr); // extract acutal size
    if (token == NULL) {
        fprintf(stderr, "Error: Missing actual_size in header\n");
        return -1;
    }
    *actual_size = strtoul(token, NULL, 10);
    token = strtok_r(NULL, HEADER_DELIM, &saveptr); // extract key size
    if (token == NULL) {
        fprintf(stderr, "Error: Missing key_size in header\n");
        return -1;
    }
    *key_size = strtoul(token, NULL, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "Error: key length size out of range\n");
        return -1;
    }
    return 0;
}

// recv() until recv_bytes == actual value size
int parse_body(int client_fd, char *buffer, int actual_size) {
    int recv_bytes = 0;
    int recv_size;
    while(recv_bytes < actual_size) {
        recv_size = recv(client_fd, buffer+recv_bytes, (actual_size+2)-recv_bytes, 0);
        if (recv_size == -1) {
            perror("recv: value");
            free(buffer);
            return -1;
        }
        recv_bytes += recv_size;
    }
    if (recv_bytes != (actual_size+2)) {
        fprintf(stderr, "Partial Recieve\nRecv: %d  Actual: %d\n", recv_bytes, actual_size);
        free(buffer);
        close(client_fd);
        return -1;
    }
    buffer[recv_bytes] = '\0';
    return 0;
}

void handle_client(void *client_fd_ptr) {
    // random delay for each client request
    srand(27);
    usleep(randnum(0.6, 1.0)); // sleep between 0.6 to 1.0 s
    int client_fd = *((int*)client_fd_ptr);
    free(client_fd_ptr);
    char *ack_n = "Error\r\n";
    char op_header[5];
    int op_bytes;
    if ((op_bytes = recv(client_fd, op_header, 5, 0)) <= 0) {
        fprintf(stderr, "message: no defined operation\n");
        send(client_fd, ack_n, strlen(ack_n), 0);
        return;
    }
    op_header[op_bytes] = '\0';
    printf("Operation:%s", op_header);
    char recv_header[HEADERSIZE];
    int header_bytes, gs_flg;
    kv_node_t *client_node;
    char *key, *buffer;
    size_t actual_size, value_size, key_size, buffer_size;
    if (strcmp(op_header, "set\r\n") == 0) {
        header_bytes = recv(client_fd, recv_header, 28, 0); // read first 28 bytes header
        if (header_bytes == -1) {
            perror("recv:header_bytes");
            send(client_fd, ack_n, strlen(ack_n), 0);
            return;
        }
        recv_header[header_bytes] = '\0';
        gs_flg = 1;  // 1 for set operation
        int header_status;
        if ((header_status = parse_header_set(recv_header, &actual_size, &value_size, &key_size) != 0)) {
            perror("header");
            send(client_fd, ack_n, strlen(ack_n), 0);
            return;
        }
        key = malloc(key_size+2);
        if (!key) {
            perror("malloc: key");
            send(client_fd, ack_n, strlen(ack_n), 0);
            return;
        }
        memset(key, 0, key_size);
        buffer_size = actual_size + 8; // 8 padding bytes
        buffer = malloc(buffer_size);
        if (!buffer) {
            perror("malloc: buffer");
            send(client_fd, ack_n, strlen(ack_n), 0);
            return;
        }
        memset(buffer, 0, buffer_size);
        int rv;
        int key_bytes;
        key_bytes = recv(client_fd, key, key_size, 0); // recv() key length bytes \r\n
        if (key_bytes == -1) {
            perror("recv:key_bytes");
            send(client_fd, ack_n, strlen(ack_n), 0);
            return;
        }
        key[key_bytes] = '\0';
        if (gs_flg == 1) {
            if ((rv = parse_body(client_fd, buffer, actual_size)) != 0) {
                perror("recv: body");
                send(client_fd, ack_n, strlen(ack_n), 0);
                return;
            }
        }
    } else {
        header_bytes = recv(client_fd, recv_header, KEYSIZE, 0); // read first 8 bytes header
        if (header_bytes == -1) {
            perror("recv:header_bytes");
            send(client_fd, ack_n, strlen(ack_n), 0);
            return;
        }
        recv_header[header_bytes] = '\0';
        gs_flg = 0; // 0 for get operation
        key = parse_header_get(recv_header);
        buffer_size = 1;
        buffer = malloc(buffer_size);
        memset(buffer, 0, buffer_size);
        value_size = 0;
    }    
    // a = create_node("A", 20, "dfsdksgjhdgnsvnitrungjnbskgb", 0);
    client_node = create_node(key, value_size, buffer, gs_flg); // returns pointer on heap
    printf("created node key: %s\tvalue size:%zu\n", key, value_size);
    pthread_mutex_lock(&hash_mutex);
    enqueue_hash(client_node, client_fd); // fifo queue
    pthread_cond_signal(&hash_cond); // signal hash_threads()
    pthread_mutex_unlock(&hash_mutex); // release lock
    // free(key);  
    // free(buffer);  
    // free(node);
}

// write ahead log
void init_log() {
    log_file = fopen("server_run.log", "a+"); // log file - append mode
    if (log_file == NULL) {
        perror("Error opening log file");
        exit(1);
    }
}

void close_log() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

void init_hash_from_log() {
    FILE *log_file = fopen("server_run.log", "r");
    if (log_file == NULL) {
        perror("Error opening log file");
        return;
    }

    char line[150];
    char key[50];
    size_t value_size, user_size;
    char value[700];  

    while (fgets(line, sizeof(line), log_file) != NULL) {
        if (sscanf(line, "SET %s %zu %zu %s", key, &user_size, &value_size, value) == 3) {
            if (value_size >= sizeof(value)) {
                fprintf(stderr, "Error: Value size too large in log line: %s", line);
                continue;
            }
            // create node
            kv_node_t *node = malloc(sizeof(kv_node_t));
            node->key = strdup(key);
            node->value = strndup(value, value_size);
            node->value_size = value_size;
            node->user_size = user_size;
            // replace existing - log read from old to new SET events
            uint32_t index = hash_string(key);
            kv_node_t *current = hash_map[index];
            while (current != NULL) {
                if (strcmp(current->key, key) == 0) {
                    free(current->value);
                    current->value = node->value;  // Reuse memory from node
                    current->value_size = node->value_size;  // Update value_size
                    current->user_size = node->user_size;  // Update user_size
                    free(node);  // Unused node
                    break;
                }
                current = current->next;
            }

            if (current == NULL) {  // Key not found, insert as new node
                insert_node(node);
            }
        }
    }

    fclose(log_file);
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

    // create connection handler threads
    for (int i = 0; i < CLIENTPOOL; i++) {
        pthread_create(&conn_pool[i], NULL, client_threads, NULL);
    }
    // create hash worker threads
    for (int j = 0; j < HASHPOOL; j++) {
        pthread_create(&hash_pool[j], NULL, hash_threads, NULL);
    }
    // init hash map
    for (int i = 0; i < TABLESIZE; i++) {
        hash_map[i] = NULL;
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

    init_log(); // set up file for logging
    // reconstruct status as of last session. last updated value retained for multiple set for same key
    init_hash_from_log(); 

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
        pthread_mutex_lock(&client_mutex);
        enqueue(client_ptr);
        pthread_cond_signal(&client_cond); // signal for incoming connection
        pthread_mutex_unlock(&client_mutex);
    }

    return 0;
}