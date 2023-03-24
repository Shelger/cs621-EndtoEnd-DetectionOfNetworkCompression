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

int bindSocket(int sockfd, struct sockaddr* my_addr, int addrlen) {
    int bd = bind(sockfd, my_addr, addrlen);
    if (bd < 0) {
        perror("Bind error.");
        exit(1);
    }
    return bd;
}

int listenSocket(int sockfd) {
    int lst = listen(sockfd, 10);
    if (lst < 0) {
        perror("Listen error.");
        exit(1);
    }
    return lst;
}

int acceptSocket(int sockfd, struct sockaddr* my_addr, int addrlen) {
    int new_fd = accept(sockfd, my_addr, (socklen_t *)&addrlen);
    if (new_fd < 0) {
        perror("Accept error.");
        exit(1);
    }
    return new_fd;
}




int main(void) {
    int sockfd;
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(8080);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket fails.");
        exit(1);
    }
    int bs = bindSocket(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr));
    int sockls = listenSocket(sockfd);
    int new_fd = acceptSocket(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr));
    char buffer[SIZE];
    int receive = recv(new_fd, buffer, SIZE, 0);
    if (receive <= 0){
      exit(1);
    }
    LOG("%s\n", buffer);
    close(sockfd);


    // // UDP starts here
    ConfigInfo cfg;
    memset(&cfg, 0, sizeof(cfg));
    parseJSON(buffer, &cfg);
    my_addr.sin_addr.s_addr = inet_addr(cfg.ip);
    my_addr.sin_port = htons(atoi(cfg.dst_port_udp));
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    LOG("%s\n", "2");
    bindSocket(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr));
    char udp_packet[SIZE];
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t client_struct_length = sizeof(client_addr);
    if (recvfrom(sockfd, udp_packet, sizeof(udp_packet), 0,
         (struct sockaddr*)&client_addr, &client_struct_length) < 0){
        printf("Couldn't receive\n");
        fprintf(stderr, "some_function failed with error %d: %s\n", errno, strerror(errno));
        return -1;
    }
    LOG("%s\n", udp_packet);
    // free(cfg);
    close(sockfd);
    return 0;
}