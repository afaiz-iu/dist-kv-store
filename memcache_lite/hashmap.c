#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define TABLESIZE 6
#define MAXKEYSIZE 100

typedef struct kv_node {
    const char *key;
    const char *value;
    size_t value_size;
    int user_size;
    struct kv_node *next;
} kv_node;

kv_node * hash_map[TABLESIZE];

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

kv_node *create_node(const char *key, int user_size, const char *value) {
    kv_node *tmp = (kv_node *)malloc(sizeof(kv_node)); // assign heap
    if (tmp == NULL) {
        perror("create: kvNode");
        exit(1);
    }
    tmp->key = key;
    tmp->value = value;
    tmp->value_size = strlen(value);
    tmp->user_size = user_size;
    tmp->next = NULL;
    return tmp;
}

bool insert_node(kv_node *kv_ptr) {
    if (kv_ptr == NULL) return false;
    uint32_t index = hash_string(kv_ptr->key);
    kv_ptr->next = hash_map[index];
    hash_map[index] = kv_ptr; // insert to head of queue
    return true;
}

kv_node *get_node(const char *key) {
    if (key == NULL) return NULL;
    uint32_t index = hash_string(key);
    kv_node *current = hash_map[index];
    while(current != NULL && (strncmp(current->key, key, MAXKEYSIZE) != 0)) {
        current = current->next;
    }
    return current;
}

bool delete_node(kv_node *kv_ptr) {
    if (kv_ptr == NULL) return false;
    uint32_t index = hash_string(kv_ptr->key);
    kv_node *current = hash_map[index];
    kv_node *prev = NULL;
    while (current->next != NULL && (strncmp(current->key, kv_ptr->key, MAXKEYSIZE) != 0)) {
        prev = current;
        current = current->next;
    }
    if (current == NULL) return false; // not found
    if (prev == NULL) {
        hash_map[index] = current->next; // delete head
    } else {
        prev->next = current->next;
    }
    return true;
}


void print_table() {
    printf("TABLE START\n");
    for(int i = 0; i < TABLESIZE; i++) {
        if (hash_map[i] == NULL) {
            printf("\t%i\t---\n", i);
        } else {
            printf("\t%i--", i);
            kv_node *tmp = hash_map[i];
            while(tmp != NULL) {
                printf("%s--", tmp->key);
                tmp = tmp->next;
            }
            printf("\n");
        }
    }
    printf("TABLE END\n");
}

int main(void) {
    
    init_map();

    kv_node *a, *b, *c, *d, *e, *f, *g, *h, *i, *j;
    a = create_node("A", 20, "dfsdksgjhdgnsvnitrungjnbskgb");
    b = create_node("B", 20, "dfsdksgjhdgnsvnitrungjnbskgb");
    c = create_node("C", 20, "dfsdksgjhdgnsvnitrungjnbskgb");
    d = create_node("D", 20, "dfsdksgjhdgnsvnitrungjnbskgb");
    e = create_node("E", 20, "dfsdksgjhdgnsvnitrungjnbskgb");
    f = create_node("F", 20, "dfsdksgjhdgnsvnitrungjnbskgb");
    g = create_node("G", 20, "dfsdksgjhdgnsvnitrungjnbskgb");
    h = create_node("H", 20, "dfsdksgjhdgnsvnitrungjnbskgb");
    i = create_node("I", 20, "dfsdksgjhdgnsvnitrungjnbskgb");
    j = create_node("J", 20, "dfsdksgjhdgnsvnitrungjnbskgb");

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

    return 0;
}