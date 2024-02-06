#include "threadqueue.h"
#include <stdlib.h>

node_t *head = NULL;
node_t *tail = NULL;

void enqueue(int *client_fd) {
    // enqueue to tail of queue
    node_t *new_node = malloc(sizeof(node_t));
    new_node->client_fd = client_fd;
    new_node->next = NULL;
    if (tail == NULL) { 
        head = new_node; // empty 
    } else {
        tail->next = new_node; // fifo
    }
    tail = new_node;
}

int *dequeue() {
    // dequeue from head of queue
    if (head == NULL) {
        // return NULL when empty
        return NULL;
    } else {
        int *fd = head->client_fd;
        node_t *temp = head;
        head = head->next;
        // check if queue empty
        if (head == NULL) tail = NULL;
        free(temp);
        return fd;
    }
}