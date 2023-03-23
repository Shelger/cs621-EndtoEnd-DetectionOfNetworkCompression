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

#define SIZE 1024


// void send_file(FILE *fp, int sockfd){
//   int n;
//   char data[SIZE] = {0};
//   while(fgets(data, SIZE, fp) != NULL) {
//     if (send(sockfd, data, sizeof(data), 0) == -1) {
//       perror("[-]Error in sending file.");
//       exit(1);
//     }
//     bzero(data, SIZE);
//   }
// }

char* read_file(char *filename) {
    FILE *fp;
    char *buffer;
    long file_size;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Error: cannot open file\n");
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);
    buffer = (char*) malloc(sizeof(char) * (file_size + 1));
    fread(buffer, sizeof(char), file_size, fp);
    buffer[file_size] = '\0'; 
    fclose(fp);
    return buffer;
}

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
    struct configInfo *cfg = malloc(sizeof(struct configInfo));
    parseJSON(buffer, cfg);

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    LOG("%s\n", cfg->ip);
    int status = getaddrinfo(cfg->ip, cfg->port_tcp, &hints, &res);
    if (status != 0) {
        perror("Get addrinfo fails.");
        exit(1);
    }
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("Socket fails.");
        exit(1);
    }
    LOG("%i\n", sockfd);
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Connection failed.");
        exit(0);
    }
    
    int sd = send(sockfd, buffer, strlen(buffer), 0);
    if (sd == -1) {
      perror("[-]Error in sending file.");
      exit(1);
    } 
  
    freeaddrinfo(res);

    char hostname[256];
    int result = gethostname(hostname, sizeof(hostname));
    if (result != 0) {
        perror("Gethosename fails.");
        exit(1);
    }
    status = getaddrinfo(cfg->ip, cfg->port_tcp, &hints, &res);
    if (status != 0) {
        perror("Get addrinfo fails.");
        exit(1);
    }
    char *buf = "aloha";
    if (sendto(sockfd, buf, strlen(buf), 0,
        res->ai_addr, res->ai_addrlen) < 0){
        printf("Can't send\n");
        return -1;
    }
//     struct addrinfo *udp_res;
//     int udp_status = getaddrinfo(hostname, "9876", &hints, &udp_res);
//     if (udp_status < 0) {
//         perror("Get addrinfo fails udp.");
//         return 1;
//     }
//     bindSocket(sockfd, udp_res);
    
//     // int val = IP_PMTUDISC_DO;
//     // setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val));

    free(buffer);
    free(cfg);
    close(sockfd);
}