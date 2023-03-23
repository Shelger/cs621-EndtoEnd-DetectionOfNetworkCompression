#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <errno.h>
#include "debug.h"
#include "parser.h"

#define SIZE 1024

// void write_file(int sockfd){
//     int n;
//     FILE *fp;
//     char buffer[SIZE];
  
//     while (1) {
//       n = recv(sockfd, buffer, SIZE, 0);
//       if (n <= 0){
//         break;
//         return;
//       }
      
      
//       bzero(buffer, SIZE);
//     }
//     return;
// }

void bindSocket(int sockfd, struct addrinfo* res) {
    int bd = bind(sockfd, res->ai_addr, res->ai_addrlen);
    if (bd < 0) {
        perror("Bind error.");
        exit(1);
    }
}

void listenSocket(int sockfd) {
    int lst = listen(sockfd, 10);
    if (lst < 0) {
        perror("Listen error.");
        exit(1);
    }
}

int acceptSocket(int sockfd, struct addrinfo* res) {
    int new_fd = accept(sockfd, (struct sockaddr *) res->ai_addr, (socklen_t *)&res->ai_addrlen);
    if (new_fd < 0) {
        perror("Accept error.");
        exit(1);
    }
    return new_fd;
}




int main(void) {
    int sockfd;
    struct addrinfo hints, *res;
    char hostname[256];
    int result = gethostname(hostname, sizeof(hostname));
    if (result != 0) {
        perror("Gethosename fails.");
        return 1;
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int status = getaddrinfo(hostname, "8080", &hints, &res);
    if (status != 0) {
        perror("Get addrinfo fails.");
        return 1;
    }
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("Socket fails.");
        exit(1);
    }
    bindSocket(sockfd, res);
    listenSocket(sockfd);
    int new_fd = acceptSocket(sockfd, res);
    char buffer[SIZE];
    int receive = recv(new_fd, buffer, SIZE, 0);
    if (receive <= 0){
      exit(1);
    }
    LOG("%s", buffer);
    close(new_fd);
    freeaddrinfo(res);


    // UDP starts here
    struct configInfo *cfg = malloc(sizeof(struct configInfo));
    parseJSON(buffer, cfg);
    LOG("%s\n", cfg->ip);
    status = getaddrinfo(hostname, cfg->dst_port_udp, &hints, &res);
    if (status != 0) {
        perror("Get addrinfo fails.");
        return 1;
    }
    char udp_packet[SIZE];
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_struct_length = sizeof(client_addr);
    if (recvfrom(sockfd, udp_packet, sizeof(udp_packet), 0,
         (struct sockaddr*)&client_addr, &client_struct_length) < 0){
        printf("Couldn't receive\n");
        return -1;
    }
    free(cfg);
    close(sockfd);
}