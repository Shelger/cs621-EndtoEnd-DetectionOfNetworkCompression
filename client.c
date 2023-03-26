#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "debug.h"
#include "parser.h"
#include <errno.h>

#define SIZE 1024

int bindSocket(int sockfd, struct sockaddr* addr, int addrlen) {
    int bd = bind(sockfd, addr, addrlen);
    if (bd < 0) {
        perror("Bind error.");
        exit(1);
    }
    return bd;
}

void createRandomSeq(uint8_t *payload, int bufsize) {
    int fd;
    
    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        perror("Open error");
        exit(EXIT_FAILURE);
    }
    
    if (read(fd, payload, bufsize) < 0) {
        perror("Read error");
        exit(EXIT_FAILURE);
    }
    
    close(fd);
}

uint8_t* create_payload(uint16_t id, size_t payload_size, int randomOrNot) {
    if (payload_size < sizeof(id)) {
        printf("Packet size should be at least 2 bytes for the ID.\n");
        return NULL;
    }

    uint8_t* payload = (uint8_t*)calloc(payload_size, sizeof(uint8_t));
    if (payload == NULL) {
        printf("Memory allocation failed.\n");
        return NULL;
    }
    if(randomOrNot == 1) {
        createRandomSeq(payload, payload_size);
    }
    payload[0] = (id >> 8) & 0xFF; // Store the high byte of the ID
    payload[1] = id & 0xFF;        // Store the low byte of the ID
    // for (size_t i = 0; i < payload_size; i++) {
    //     printf("%u", (unsigned int)payload[i]);
    // }
    return payload;
}

void sendUdp(ConfigInfo *cfg, int sockfd, struct sockaddr_in *addr, int sizeAddr, int randomOrNot)
{
    for(int i = 0; i < cfg->num_udp_packet; i++) {
        uint8_t* payload = create_payload((uint16_t)i, cfg->size_udp, randomOrNot);
        if (sendto(sockfd, payload, cfg->size_udp, 0, (struct sockaddr*)addr, sizeAddr) < 0){
            printf("Can't send\n");
            exit(1);
        }
        // LOG("%u\n", payload[1]);
        free(payload);
    }
}

int main() {
    char *buffer = read_file("config.json");
    char *copy_buf = malloc(SIZE);
    strcpy(copy_buf, buffer);
    ConfigInfo cfg;
    memset(&cfg, 0, sizeof(cfg));
    parseJSON(copy_buf, &cfg);


    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(cfg.ip);
    server_addr.sin_port = htons(8081);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket fails.");
        exit(1);
    }
    socklen_t sl = sizeof(server_addr);
    if (connect(sockfd, (struct sockaddr*)&server_addr, sl) < 0) {
        perror("Connection failed.");
        exit(0);
    }
    
    int sd = send(sockfd, buffer, strlen(buffer), 0);
    if (sd == -1) {
      perror("[-]Error in sending file.");
      exit(1);
    } 
    close(sockfd);

    // UDP starts below
    struct sockaddr_in udp_addr;
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = inet_addr(cfg.ip);
    udp_addr.sin_port = htons(cfg.dst_port_udp);
    int udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    int val = IP_PMTUDISC_DO;
    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;;
    client_addr.sin_port = htons(cfg.src_port_udp);
    bindSocket(udp_sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr));
    setsockopt(udp_sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val));

    sendUdp(&cfg, udp_sockfd, &udp_addr, sizeof(udp_addr), 0);
    sleep(15);
    sendUdp(&cfg, udp_sockfd, &udp_addr, sizeof(udp_addr), 1);
    close(udp_sockfd);

    // Phase3
    sleep(20);
    // struct sockaddr_in p3_addr;
    // memset(&p3_addr, 0, sizeof(p3_addr));
    // p3_addr.sin_family = AF_INET;
    // p3_addr.sin_addr.s_addr = inet_addr(cfg.ip);
    // p3_addr.sin_port = htons(atoi(cfg.dst_port_udp));
    int p3_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (p3_sockfd < 0) {
        perror("Socket fails.");
        exit(1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(cfg.ip);
    server_addr.sin_port = htons(cfg.port_tcp);
    int server_fd = connect(p3_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (server_fd < 0) {
        perror("Connection failed.");
        exit(0);
    }

    char check = '0';
    int receive = recv(p3_sockfd, &check, 1, 0);
    if (receive < 0){
      exit(1);
    }
    LOG("%c\n", check);
    if(check == '0') printf("No compression was detected.\n");
    else printf("Compression detected!\n");
    close(p3_sockfd);
    free(copy_buf);
}