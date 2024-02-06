#ifndef HASHMAP_H_
#define HASHMAP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define MAXKEYSIZE 12
#define TABLESIZE 5


struct kv_node {
    const char *key;
    char *value;
    size_t value_size;
    size_t user_size;
    unsigned char oper_flg;
    struct kv_node *next;
};

typedef struct kv_node kv_node_t;

extern kv_node_t *hash_map[TABLESIZE];
extern pthread_mutex_t hash_mutex;

void init_map();
uint32_t hash_string(const char *str);
kv_node_t *create_node(const char *key, size_t user_size, char *value, unsigned char oper_flg);
bool insert_node(kv_node_t *kv_ptr);
// bool insert_node1(kv_node_t *kv_ptr);
kv_node_t *get_node(const char *key);
bool delete_node(kv_node_t *kv_ptr);
void print_table(void);

struct node_hash {
    kv_node_t *data;
    int sock_fd;
    struct node_hash *next;
};
typedef struct node_hash node_hash_t;

void enqueue_hash(kv_node_t *tmp, int sock_fd);
node_hash_t *dequeue_hash();
#endif 