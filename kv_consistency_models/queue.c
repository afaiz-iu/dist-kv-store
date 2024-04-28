#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

node_t *head = NULL;
node_t *tail = NULL;

node_t *create_node(pid_t pid, int port_num, int ack_flg, int ack_count, char *buffer, size_t buf_len, unsigned int lc, unsigned int uid, int nprocs) {
    node_t *n = malloc(sizeof(node_t));
    if (n == NULL) {
        return NULL;
    }
    n->port_num = port_num;
    n->ack_flg = ack_flg;
    n->ack_count = ack_count;
    n->pid = pid;
    n->lc = lc;
    n->uid = uid;
    n->nprocs = nprocs;
    n->next = NULL;  // set to NULL when creating node
    n->buffer = malloc(buf_len + 1);
    if (n->buffer == NULL) {
        free(n); // free
        return NULL;
    }
    memcpy(n->buffer, buffer, buf_len);
    n->buf_len = buf_len; // update actual bytes copied
    return n;
}

void enqueue(node_t *new_node) {
    // enqueue to tail of queue
    if (tail == NULL) { 
        head = new_node; // empty 
    } else {
        tail->next = new_node; // fifo
    }
    tail = new_node;
}

void print_queue(pid_t base_pid) {
    node_t *current = head;
    while (current != NULL) {
        printf("[%d] base port: %d, LC: %u, Event ID: %u, ack_flg: %d, Msg Length: %zu, Msg: %s\n",
               base_pid, current->port_num, current->lc, current->uid, current->ack_flg, current->buf_len, current->buffer);
        current = current->next;
    }
}

node_t* find_node(unsigned int uid) {
    node_t* current = head;
    while (current != NULL) {
        if (current->uid == uid) {
            return current;
        }
        current = current->next;
    }
    return NULL; // not found
}


// insertion sort
void order_queue() {
    if (head == NULL || head->next == NULL) {
        return; // empty queue
    }
    node_t *sorted = NULL; // starting pointer

    while (head != NULL) {
        node_t *current = head; // current node
        head = head->next; 

        // iterate and find the correct position of current
        if (sorted == NULL || sorted->lc > current->lc || 
            (sorted->lc == current->lc && sorted->pid < current->pid)) {
            // insert at the beginning if sorted list is empty
            // or if current node's lc is smaller
            // or if lc is the same but current node's pid is larger
            current->next = sorted;
            sorted = current;
        } else {
            // search for node after which the current node should be inserted
            node_t *temp = sorted;
            while (temp->next != NULL && 
                   (temp->next->lc < current->lc || 
                   (temp->next->lc == current->lc && temp->next->pid >= current->pid))) {
                temp = temp->next;
            }
            // insert current node
            current->next = temp->next;
            temp->next = current;
        }
    }

    // update head
    head = sorted;

    // update tail
    tail = head;
    while (tail != NULL && tail->next != NULL) {
        tail = tail->next;
    }
}

// dequeue node for the given uid
void dequeue(unsigned int uid) {
    if (head == NULL) return; // empty
    node_t *temp = head, *prev = NULL;

    // if head == uid delete
    if (temp != NULL && temp->uid == uid) {
        head = temp->next; 
        free(temp->buffer); // free buffer
        free(temp); // free temp
        return;
    }

    // search list
    while (temp != NULL && temp->uid != uid) {
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL) return; // not found

    // update prev pointer 
    prev->next = temp->next;
    // update tail if last element
    if (temp == tail) {
        tail = prev;
    }

    free(temp->buffer); // free buffer
    free(temp); // free temp node
}
