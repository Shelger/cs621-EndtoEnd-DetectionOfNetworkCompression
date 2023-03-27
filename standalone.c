#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/socket.h>
#include <threads.h>
#include <sys/time.h>
#include <math.h>
#include <fcntl.h>
#include "parser.h"

// unsigned short checksum(const char *buf, unsigned size)
// {
// 	unsigned sum = 0, i;

// 	/* Accumulate checksum */
// 	for (i = 0; i < size - 1; i += 2)
// 	{
// 		unsigned short word16 = *(unsigned short *) &buf[i];
// 		sum += word16;
// 	}

// 	/* Handle odd-sized case */
// 	if (size & 1)
// 	{
// 		unsigned short word16 = (unsigned char) buf[i];
// 		sum += word16;
// 	}

// 	/* Fold to get the ones-complement result */
// 	while (sum >> 16) sum = (sum & 0xFFFF)+(sum >> 16);

// 	/* Invert to get the negative in ones-complement arithmetic */
// 	return ~sum;x
// }

unsigned short csum(unsigned short *buf, int nwords) {
    unsigned long sum;
    for (sum = 0; nwords > 0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

int
send_syn_packet(ConfigInfo *cfg, int port_num) {
    int s = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
    if (s == -1) {
        perror("Failed to create socket");
        exit(1);
    }

    // The IP header and TCP header (with options) buffer
    char packet[40];
    memset(packet, 0, 40);

    // IP header
    struct iphdr *iph = (struct iphdr *) packet;

    // TCP header
    struct tcphdr *tcph = (struct tcphdr *) (packet + sizeof(struct iphdr));

    struct sockaddr_in dst;
    dst.sin_family = AF_INET;
    dst.sin_port = htons(port_num);
    dst.sin_addr.s_addr = inet_addr(cfg->ip);

    // IP header fields
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr);
    iph->id = htonl(54321);
    iph->frag_off = 0;
    iph->ttl = cfg->ttl;
    iph->protocol = IPPROTO_TCP;
    iph->check = 0;
    iph->saddr = inet_addr(cfg->src_ip);
    iph->daddr = dst.sin_addr.s_addr;

    iph->check = csum((unsigned short *) packet, iph->tot_len >> 1);

    // TCP header fields
    tcph->source = htons(1996);
    tcph->dest = htons(port_num);
    tcph->seq = 0;
    tcph->ack_seq = 0;
    tcph->doff = 5;
    tcph->fin = 0;
    tcph->syn = 1;
    tcph->rst = 0;
    tcph->psh = 0;
    tcph->ack = 0;
    tcph->urg = 0;
    tcph->window = htons(5840);
    tcph->check = 0;
    tcph->urg_ptr = 0;

    // IP pseudo-header for TCP checksum calculation
    struct {
        uint32_t src_addr;
        uint32_t dest_addr;
        uint8_t placeholder;
        uint8_t protocol;
        uint16_t tcp_length;
    } psh;

    psh.src_addr = inet_addr(cfg->src_ip);
    psh.dest_addr = dst.sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(sizeof(struct tcphdr));

    char *pseudo_packet = (char *) malloc(sizeof(psh) + sizeof(struct tcphdr));
    memcpy(pseudo_packet, &psh, sizeof(psh));
    memcpy(pseudo_packet + sizeof(psh), tcph, sizeof(struct tcphdr));

    tcph->check = csum((unsigned short *)pseudo_packet, (sizeof(psh) + sizeof(struct tcphdr)) >> 1);

    // Send the packet
    if (sendto(s, packet, iph->tot_len, 0, (struct sockaddr *)&dst, sizeof(dst)) < 0) {
        perror("sendto failed");
    } else {
        printf("Packet sent successfully\n");
    }

    free(pseudo_packet);
}

struct pthreadScoket {
    int fd;
    struct timeval tval;
};

int
receive_rst_packet(void *a) {
    struct pthreadScoket* ps = a;
    char *buffer = malloc(1000);
    struct sockaddr_in target_addr;
    socklen_t target_addr_len = sizeof(target_addr);
    memset(&target_addr, 0, sizeof(target_addr));
    struct timeval start, end;
    memset(&(ps->tval), 0, sizeof(ps->tval));
    memset(&end, 0, sizeof(end));
    gettimeofday(&start, NULL);
    
    while (1) {
        gettimeofday(&end, NULL);
        if(((end.tv_sec - start.tv_sec) + ((end.tv_usec - start.tv_usec) / 1000000.0)) > 5) {
            printf("RST Timeout!");
            break;
        }
        ssize_t received_size = recvfrom(ps->fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&target_addr, &target_addr_len);
        if (received_size < 0) {
            perror("recvfrom");
            close(ps->fd);
            exit(1);
        }
        // Verify the received packet
        struct iphdr *received_ip_header = (struct iphdr *)buffer;
        struct tcphdr *received_tcp_header = (struct tcphdr *)(buffer + received_ip_header->ihl * 4);
        // Check if the received packet is the RST response from the target
        if (received_ip_header->protocol == IPPROTO_TCP &&
            // received_tcp_header->dest == htons(SOURCE_PORT) &&
            received_tcp_header->rst == 1) {
            printf("Received RST response from target\n");
            gettimeofday(&(ps->tval), NULL);
            break;
        }
    }
    free(buffer);
    return 0;
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

double
oneRound(ConfigInfo *cfg, int mode, int port_num) {
    struct sockaddr_in udp_addr;
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = inet_addr(cfg->ip);
    udp_addr.sin_port = htons(cfg->dst_port_udp);
    int udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    int val = IP_PMTUDISC_DO;
    // server_addr.sin_port = htons(cfg->src_port_udp);
    setsockopt(udp_sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val));
    struct pthreadScoket start, end;
    int sockfd = send_syn_packet(cfg, port_num);
    start.fd = sockfd;
    thrd_t t1;
    thrd_create(&t1, receive_rst_packet, &start);
    // bindSocket(udp_sockfd, (struct sockaddr*)&client_addr, sizeof(server_addr));
    sendUdp(cfg, udp_sockfd, &udp_addr, sizeof(udp_addr), mode);
    sockfd = send_syn_packet(cfg, port_num);
    thrd_t t2;
    end.fd = sockfd;
    thrd_create(&t2, receive_rst_packet, &end);
    thrd_join(t1, NULL);
    thrd_join(t2, NULL);

    if((start.tval.tv_sec == 0 && start.tval.tv_usec == 0) || (end.tval.tv_sec == 0 && end.tval.tv_usec == 0)) {
        return -1;
    }
    double elapsed_time = (end.tval.tv_sec - start.tval.tv_sec) + ((end.tval.tv_usec - start.tval.tv_usec) / 1000000.0);
    close(udp_sockfd);
    close(sockfd);
    return elapsed_time;
}


int main() {
    char *buffer = read_file("config.json");
    ConfigInfo cfg;
    memset(&cfg, 0, sizeof(cfg));
    parseJSON(buffer, &cfg);

    double res1 = oneRound(&cfg, 0, cfg.head_syn);
    if(res1 < 0) {
        printf("Failed to detect due to insufficient information.");
        return 1;
    }
    double res2 = oneRound(&cfg, 1, cfg.tail_syn);
    if(res2 < 0) {
        printf("Failed to detect due to insufficient information.");
        return 1;
    }

    double result = res1 - res2;
    if (fabs(result) > 0.1) {
        ("Compression detected!\n");
    } else {
        printf("No compression was detected.\n");
    }
    return 0;
}