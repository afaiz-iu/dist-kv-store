#ifndef QUEUE_H_
#define QUEUE_H_
#include <stdlib.h>

struct node {
    pid_t pid; // process id which issues the message
    int port_num; // port number which issues the message
    int ack_flg; // set this to 1 when ack is sent to sender on handling broadcast msg
    int ack_count; // counter for total ack msg recieved
    char *buffer; // buffer to store msg
    size_t buf_len;
    unsigned int lc; // lamport clock ts
    unsigned int uid; // uid (generate unique for issued node) pass this in p2p ack to sender
    int nprocs; // number of processes
    struct node *next;
};
typedef struct node node_t;

node_t *create_node(pid_t pid, int port_num, int ack_flg, int ack_count, char *buffer, size_t buf_len, unsigned int lc, unsigned int uid, int nprocs);
void enqueue(node_t *new_node);
void print_queue(pid_t base_pid);
void order_queue();
node_t* find_node(unsigned int uid);
void dequeue(unsigned int uid);

#endif