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
#include <sys/time.h>
#include <math.h>
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

int setTimeout(int sockfd) {
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    int output = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
    if (output < 0) {
        perror("Setsockopt error");
        exit(EXIT_FAILURE);
    }
    return output;
}

double recordTime(int sockfd, struct sockaddr_in *client_addr, socklen_t client_struct_length, ConfigInfo *cfg) {
    struct timeval start, end;
    double elapsed_time;
    memset(&start, 0, sizeof(start));
    memset(&start, 0, sizeof(start));
    setTimeout(sockfd);
    for (int i = 0; i < cfg->num_udp_packet; i++) {
        uint8_t udp_packet[1000];
        int bytes_received = recvfrom(sockfd, udp_packet, cfg->size_udp, 0, (struct sockaddr*)client_addr, &client_struct_length);
        if (bytes_received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("Timeout occurred\n");
                LOG("%s\n", "timeout");
                break;
            } else {
                // LOG("%s\n", "boom.");
                perror("Recvfrom error");
                exit(1);
            }
        } else {
            if (start.tv_sec == 0 && start.tv_usec == 0) {
                gettimeofday(&start, NULL);
            }
            gettimeofday(&end, NULL);
        }
    }
    elapsed_time = (end.tv_sec - start.tv_sec) + ((end.tv_usec - start.tv_usec) / 1000000.0);
    LOG("Elapsed time: %f seconds\n", elapsed_time);
    // LOG("Span time: %f seconds\n", span);
    if(elapsed_time + 10 < cfg->inter_time) {
        sleep(cfg->inter_time - 10 - elapsed_time);
    }
    return elapsed_time;
}




int main(void) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("10.10.12.99");
    server_addr.sin_port = htons(8080);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket fails.");
        exit(1);
    }
    int reuse_addr = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) < 0) {
        perror("setsockopt");
        close(sockfd);
        exit(1);
    }
    int bs = bindSocket(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    int sockls = listenSocket(sockfd);
    int new_fd = acceptSocket(sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr));
    char *buffer = malloc(1024);
    int receive = recv(new_fd, buffer, SIZE, 0);
    if (receive <= 0){
      exit(1);
    }
    LOG("%s\n", buffer);
    close(new_fd);
    close(sockfd);


    // // UDP starts here
    ConfigInfo cfg;
    memset(&cfg, 0, sizeof(cfg));
    parseJSON(buffer, &cfg);
    struct sockaddr_in udp_addr;
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(atoi(cfg.dst_port_udp));
    int new_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    bindSocket(new_sockfd, (struct sockaddr*)&udp_addr, sizeof(udp_addr));
    // struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t client_struct_length = sizeof(client_addr);

    double gap1 = recordTime(new_sockfd, &client_addr, client_struct_length, &cfg);
    double gap2 = recordTime(new_sockfd, &client_addr, client_struct_length, &cfg);
    double result = gap2 - gap1;
    // free(cfg);
    close(new_sockfd);
    free(buffer);

    // struct sockaddr_in p3_addr;
    // memset(&p3_addr, 0, sizeof(p3_addr));
    // p3_addr.sin_family = AF_INET;
    // p3_addr.sin_addr.s_addr = inet_addr(cfg.ip);
    // p3_addr.sin_port = htons(atoi(cfg.dst_port_udp));
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket fails.");
        exit(1);
    }
    server_addr.sin_port = htons(8765);
    bs = bindSocket(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    LOGP("Listening...\n");
    sockls = listenSocket(sockfd);
    memset(&client_addr, 0, sizeof(client_addr));
    new_fd = acceptSocket(sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr));
    char check = '0';
    if (fabs(result) > 0.1) {
        check = '1';
    }
    int sd = send(new_fd, &check, 1, 0);
    if (sd == -1) {
        perror("[-]Error in sending file.");
        exit(1);
    }
    close(new_fd);
    close(sockfd);
    return 0;
}