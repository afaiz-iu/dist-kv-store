#include "threadqueue.h"
#include <stdlib.h>

node_t *head = NULL;
node_t *tail = NULL;

void enqueue(int *client_fd) {
    // enqueue to head of queue
    node_t *new_node = malloc(sizeof(node_t));
    new_node->client_fd = client_fd;
    new_node->next = NULL;
    if (tail == NULL) { // empty 
        head = new_node;
    } else {
        tail->next = new_node;
    }
    tail = new_node;
}

int *dequeue() {
    if (head == NULL) {
        // return NULL when queue empty
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