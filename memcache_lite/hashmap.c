#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "hashmap.h"

node_hash_t *head_hash = NULL;
node_hash_t *tail_hash = NULL;

void init_map() {
    for (int i = 0; i < TABLESIZE; i++) {
        hash_map[i] = NULL;
    }
}

// djb2 hash function 
// returns 32 bit unsigned int
uint32_t hash_string(const char *str) {
    uint32_t hash = 5381;  // Initial hash value
    while (*str) {
        hash = (hash << 5) + (*str++);  // hash * 33 + c
    }
    return hash % TABLESIZE;
}

kv_node_t *create_node(const char *key, size_t user_size, char *value, unsigned char oper_flg) {
    kv_node_t *node = (kv_node_t *)malloc(sizeof(kv_node_t)); // assign heap
    if (node == NULL) {
        perror("kvNode heap");
        exit(1);
    }
    node->key = strdup(key);
    node->value = strdup(value);
    node->value_size = strlen(value);
    node->user_size = user_size;
    node->oper_flg = oper_flg; // 0 for get, 1 for set
    node->next = NULL;
    return node;
}

bool insert_node(kv_node_t *kv_ptr) {
    if (kv_ptr == NULL) return false;
    char *key_copy = strdup(kv_ptr->key);
    uint32_t index = hash_string(key_copy);
    kv_ptr->next = hash_map[index];
    hash_map[index] = kv_ptr; // insert to head of queue
    // free(kv_ptr);  // never free if not making a struct copy
    return true;
}

kv_node_t *get_node(const char *key) {
    if (key == NULL) return NULL;
    uint32_t idx = hash_string(key);
    int index = idx;
    if (hash_map[index] == NULL) {
        return NULL;
    }
    kv_node_t *current = hash_map[index];
    size_t key_len = strlen(key);
    while(current != NULL && (strncmp(current->key, key, key_len) != 0)) {
        printf("current key:%s\n", current->key);
        current = current->next;
    }
    return current;
}

void enqueue_hash(kv_node_t *tmp, int sock_fd) {
    node_hash_t *new_node = (node_hash_t*)malloc(sizeof(node_hash_t));
    if (new_node == NULL) {
        perror("error allocaating node_hash_t");
        return;
    }
    new_node->data = tmp;
    new_node->next = NULL;
    new_node->sock_fd = sock_fd;
    if (tail_hash == NULL) { 
        head_hash = new_node; // empty queue
    } else {
        tail_hash->next = new_node; // fifo
    }
    tail_hash = new_node; // update tail
}

node_hash_t *dequeue_hash() {
    if (head_hash == NULL) { // empty
        return NULL;
    } else {
        node_hash_t *tmp = head_hash;
        head_hash = head_hash->next;
        if (head_hash == NULL) tail_hash = NULL;
        // free(tmp);
        return tmp; // pointer to kv_node_t
    }
}

void print_table(void) {
    printf("TABLE START\n");
    for(int i = 0; i < TABLESIZE; i++) {
        if (hash_map[i] == NULL) {
            printf("\t%i\t---\n", i);
        } else {
            printf("\t%i--", i);
            kv_node_t *tmp = hash_map[i];
            while(tmp != NULL) {
                printf("%s--", tmp->key);
                tmp = tmp->next;
            }
            printf("\n");
        }
    }
    printf("TABLE END\n");
}

bool delete_node(kv_node_t *kv_ptr) {
    if (kv_ptr == NULL) return false;
    char *key_copy = strdup(kv_ptr->key);
    size_t key_len = strlen(key_copy);
    uint32_t index = hash_string(key_copy);
    if (hash_map[index] == NULL) return false;
    kv_node_t *current = hash_map[index];
    kv_node_t *prev = NULL;
    if (head_hash == NULL) return false;
    while ((strncmp(key_copy, current->key, key_len) != 0) && (current != NULL)) {
        prev = current;
        current = current->next;

    }
    if (current == NULL) return false;
    if (prev == NULL){
        hash_map[index] = current->next; // delete head
    } else { 
        prev->next = current->next;
        free(current); // free
    }
    return true;
}
/*
int main(void) {
    
    init_map();

    kv_node_t *a, *b, *c, *d, *e, *f, *g, *h, *i, *j;
    a = create_node("A", 20, "dfsdksgjhdgnsvnitrungjnbskgb", 1);
    printf("%s %zu %zu %s %u\n", a->key, a->user_size, a->value_size, a->value, a->oper_flg);
    b = create_node("B", 20, "dfsdksgjhdgnsvnitrungjnbskgb", 1);
    c = create_node("C", 20, "dfsdksgjhdgnsvnitrungjnbskgb", 1);
    d = create_node("D", 20, "dfsdksgjhdgnsvnitrungjnbskgb", 1);
    e = create_node("E", 20, "dfsdksgjhdgnsvnitrungjnbskgb", 1);
    printf("%s %zu %zu %s\n", e->key, e->user_size, e->value_size, e->value);
    f = create_node("F", 20, "dfsdksgjhdgnsvnitrungjnbskgb", 1);
    g = create_node("G", 20, "dfsdksgjhdgnsvnitrungjnbskgb", 1);
    h = create_node("H", 20, "dfsdksgjhdgnsvnitrungjnbskgb", 1);
    i = create_node("I", 20, "dfsdksgjhdgnsvnitrungjnbskgb", 1);
    j = create_node("J", 20, "dfsdksgjhdgnsvnitrungjnbskgb", 1);

    printf("%s\n", a->key);
    print_table();
    insert_node(a);
    insert_node(b);
    insert_node(c);
    insert_node(d);
    insert_node(e);
    insert_node(f);
    insert_node(g);
    print_table();
    printf("after print()");
    delete_node(g);
    print_table();
    insert_node(h);
    insert_node(i);
    insert_node(j);
    // print_table();
    printf("before get\n");
    get_node(a->key);
    printf("after get\n");
    printf("%s\n", a->key);

    // free(a);
    // free(b);
    // free(c);
    // free(d);
    // free(e);
    // free(f);
    // free(h);
    // free(i);
    // free(j);

    return 0;
}
*/