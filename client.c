#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "debug.h"
#include "parser.h"
#include <errno.h>

#define SIZE 1024

void bindSocket(int sockfd, struct addrinfo* res) {
    int bd = bind(sockfd, res->ai_addr, res->ai_addrlen);
    if (bd < 0) {
        perror("Bind error.");
        exit(1);
    }
}

// void sendUDP(int sockfd) {
//     if (sendto(socket_desc, server_message, strlen(server_message), 0,
//          (struct sockaddr*)&client_addr, client_struct_length) < 0){
//         printf("Can't send\n");
//         return -1;
//     }
//     1
// }


int main() {
    char *buffer = read_file("config.json");
    char *copy_buf = malloc(SIZE);
    strcpy(copy_buf, buffer);
    ConfigInfo cfg;
    memset(&cfg, 0, sizeof(cfg));
    parseJSON(copy_buf, &cfg);

    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(8080);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket fails.");
        exit(1);
    }
    socklen_t sl = sizeof(my_addr);
    if (connect(sockfd, (struct sockaddr*)&my_addr, sl) < 0) {
        perror("Connection failed.");
        exit(0);
    }
    LOG("%i connected\n", sockfd);
    
    int sd = send(sockfd, buffer, strlen(buffer), 0);
    if (sd == -1) {
      perror("[-]Error in sending file.");
      exit(1);
    } 
    close(sockfd);
  
    // freeaddrinfo(res);

    // char hostname[256];
    // int result = gethostname(hostname, sizeof(hostname));
    // if (result != 0) {
    //     perror("Gethosename fails.");
    //     exit(1);
    // }
    my_addr.sin_addr.s_addr = inet_addr(cfg.ip);
    my_addr.sin_port = htons(atoi(cfg.dst_port_udp));
    int udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    char *buf = "aloha";
    if (sendto(udp_sockfd, buf, strlen(buf), 0,
        (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0){
        printf("Can't send\n");
        return -1;
    }
// //     struct addrinfo *udp_res;
// //     int udp_status = getaddrinfo(hostname, "9876", &hints, &udp_res);
// //     if (udp_status < 0) {
// //         perror("Get addrinfo fails udp.");
// //         return 1;
// //     }
// //     bindSocket(sockfd, udp_res);
    
// //     // int val = IP_PMTUDISC_DO;
// //     // setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val));

//     free(buffer);
//     free(copy_buf);
    // freeConfig(&cfg);
    // close(sockfd);
}