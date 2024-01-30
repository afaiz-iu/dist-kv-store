#ifndef THREADQUEUE_H_
#define THREADQUEUE_H_

struct node {
    struct node *next;
    int *client_fd;
};
typedef struct node node_t;

void enqueue(int *client_fd);
int *dequeue();

#endif