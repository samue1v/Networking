#include <stdio.h>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include "netw.hpp"


int main(int argc, char * argv[]){
    int sockfd, new_sockfd, yes=1;
    struct sockaddr_in host_addr, client_addr; // My address information
    socklen_t sin_size;
    printf("Accepting web requests on port %d\n", PORT);
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        fatal("in socket");
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        fatal("setting socket option SO_REUSEADDR");
    host_addr.sin_family = AF_INET; // Host byte order
    host_addr.sin_port = htons(PORT); // Short, network byte order
    host_addr.sin_addr.s_addr = INADDR_ANY; // Automatically fill with my IP.
    memset(&(host_addr.sin_zero), '\0', 8); // Zero the rest of the struct.
    bind(sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr));
    if (!sockfd)
        fatal("binding to socket");
    if (listen(sockfd, 20) == -1)
        fatal("listening on socket");
    while(1) { // Accept loop.
        sin_size = sizeof(struct sockaddr_in);
        new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
        if(new_sockfd == -1)
        fatal("accepting connection");
        handle_connection(new_sockfd, &client_addr);
    }
    return 0;
}
