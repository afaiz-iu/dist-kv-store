/*
 ** simple tcp server 
 ** accepts hostname as argument
 ** creates a socket connection to the host
 ** converts host ip from pton and print
 ** close socket and exit
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    printf("inside main\n");
    struct addrinfo hints, *results, *ptr;
    int status;
    char *ipstr[INET6_ADDRSTRLEN]; // 128 bit for IPv6

    // set up struct for addrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((status = getaddrinfo(argv[1], NULL, &hints, &results) != 0)) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    ptr = results;
    while(ptr->ai_next != NULL) {
        void *addr;
        char *ipver;
        // accept first socket possible
        // check for IPv4 or IPv6
        if (ptr->ai_family == AF_INET) {
            // cast sockaddr to sockaddr_in
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)ptr->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        }
        else {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)ptr->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }
        //convert addr from network to presentation
        inet_ntop(ptr->ai_family, addr, ipstr, sizeof(ipstr));
        printf("%s: %s\n", ipver, ipstr);
        break;
    }

    freeaddrinfo(results);
    return 0;
}