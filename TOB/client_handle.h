#ifndef CLIENT_HANDLE_H
#define CLIENT_HANDLE_H

#include "queue.h"

void handle_client(int *client_fd_ptr, int port_num, int num_process, unsigned int *lc);
int total_broadcast(node_t *node, unsigned int *lc);
void handle_broadcast(int *client_fd_ptr, int num_process, unsigned int *lc);
void handle_issue(int *client_fd_ptr, int num_process, unsigned int *lc, int base_port);
int recv_buffer(int *pid_fd_ptr, char *buffer, int *buf_len);
int send_buffer(int *pid_fd_ptr, char *buffer, size_t *buf_len);
void send_p2p_ack(int sender_port, int base_pid, unsigned int uid);
void broadcast_ready_message(node_t *node, unsigned int *lc);
void handle_ack(int *client_fd_ptr, unsigned int *lc);


#endif