#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include "queue.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BASEPORT 10000 // base port for the first process
#define NUMPROC 2 // default number of sub processes
#define BACKLOG 8 // number of backlog connections to listen
#define MAXMSGSIZE 4098 // maximum number of message bytes recv from client
#define PORTSFILE "listening_ports.txt" // file to store list of ports