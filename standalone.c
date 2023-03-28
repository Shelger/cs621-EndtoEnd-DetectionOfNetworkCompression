#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/time.h>
#include <math.h>
#include <fcntl.h>
#include "parser.h"
#include "debug.h"

#define DATAGRAM_LEN 2048
#define OPT_SIZE 20

struct pseudo_header
{
	u_int32_t source_address;
	u_int32_t dest_address;
	u_int8_t placeholder;
	u_int8_t protocol;
	u_int16_t tcp_length;
};

unsigned short
checksum(const char *buf, unsigned size)
{
	unsigned sum = 0, i;

	/* Accumulate checksum */
	for (i = 0; i < size - 1; i += 2)
	{
		unsigned short word16 = *(unsigned short *) &buf[i];
		sum += word16;
	}

	/* Handle odd-sized case */
	if (size & 1)
	{
		unsigned short word16 = (unsigned char) buf[i];
		sum += word16;
	}

	/* Fold to get the ones-complement result */
	while (sum >> 16) sum = (sum & 0xFFFF)+(sum >> 16);

	/* Invert to get the negative in ones-complement arithmetic */
	return ~sum;
}

int 
setTimeout(int sockfd) 
{
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    int output = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
    if (output < 0) 
    {
        perror("Setsockopt error");
        exit(EXIT_FAILURE);
    }
    return output;
}


int
send_syn_packet(ConfigInfo *cfg, int port_num, int s)
{
    int one = 1;
    if (setsockopt(s, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) 
    {
        perror("Failed to set IP_HDRINCL option");
        exit(1);
    }

    char *datagram = calloc(DATAGRAM_LEN, sizeof(char));

	// required structs for IP and TCP header
	struct iphdr *iph = (struct iphdr*)datagram;
	struct tcphdr *tcph = (struct tcphdr*)(datagram + sizeof(struct iphdr));
	struct pseudo_header psh;

	// IP header configuration
	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr) + OPT_SIZE;
	iph->id = htonl(rand() % 65535); 
	iph->frag_off = 0;
	iph->ttl = cfg->ttl;
	iph->protocol = IPPROTO_TCP;
	iph->check = 0; // correct calculation follows later
	iph->saddr = inet_addr(cfg->src_ip);
	iph->daddr = inet_addr(cfg->ip);

	// TCP header configuration
	tcph->source = htons(1996+s);
	tcph->dest = htons(port_num);
	tcph->seq = htonl(rand() % 4294967295);
	tcph->ack_seq = htonl(0);
	tcph->doff = 10;
	tcph->fin = 0;
	tcph->syn = 1;
	tcph->rst = 0;
	tcph->psh = 0;
	tcph->ack = 0;
	tcph->urg = 0;
	tcph->check = 0; // correct calculation follows later
	tcph->window = htons(5840);
	tcph->urg_ptr = 0;

	// TCP pseudo header for checksum calculation
	psh.source_address = inet_addr(cfg->src_ip);
	psh.dest_address = inet_addr(cfg->ip);
	psh.placeholder = 0;
	psh.protocol = IPPROTO_TCP;
	psh.tcp_length = htons(sizeof(struct tcphdr) + OPT_SIZE);
	int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr) + OPT_SIZE;
	// fill pseudo packet
	char* pseudogram = malloc(psize);
	memcpy(pseudogram, (char*)&psh, sizeof(struct pseudo_header));
	memcpy(pseudogram + sizeof(struct pseudo_header), tcph, sizeof(struct tcphdr) + OPT_SIZE);


	tcph->check = checksum((const char*)pseudogram, psize);
	iph->check = checksum((const char*)datagram, iph->tot_len);

    // Send the packet
    struct sockaddr_in dst;
    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = inet_addr(cfg->ip);
    dst.sin_port = htons(port_num);
    LOG("size of dst is %ld\n", sizeof(dst));
    if (sendto(s, datagram, iph->tot_len, 0, (struct sockaddr *)&dst, sizeof(dst)) < 0) 
    {
        perror("sendto failed");
    } 
    else 
    {
        printf("Packet sent successfully\n");
    }

    free(pseudogram);
    free(datagram);
    return s;
}



void 
*receive_rst(void *arg) 
{
    struct timeval *record = arg;
    int s = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
    if (s == -1) 
    {
        perror("Failed to create socket");
        pthread_exit(NULL);
    }
    setTimeout(s);

    struct sockaddr_in saddr;
    socklen_t saddr_len = sizeof(saddr);
    char buffer[4096];

    while (1) 
    {
        ssize_t pkt_size = recvfrom(s, buffer, sizeof(buffer), 0, (struct sockaddr *)&saddr, &saddr_len);
        if (pkt_size < 0) 
        {
        perror("recvfrom");
        continue;
        }

        struct iphdr *iph = (struct iphdr *)buffer;
        struct tcphdr *tcph = (struct tcphdr *)(buffer + (iph->ihl << 2));
        
        if (tcph->rst == 1) 
        {
        printf("Received RST packet from %s:%d\n", inet_ntoa(saddr.sin_addr), ntohs(tcph->source));
        gettimeofday(record, NULL);
        break;
        }
    }

    close(s);
    pthread_exit(NULL);
}


void 
createRandomSeq(uint8_t *payload, int bufsize) 
{
    int fd;
    
    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) 
    {
        perror("Open error");
        exit(EXIT_FAILURE);
    }
    
    if (read(fd, payload, bufsize) < 0) 
    {
        perror("Read error");
        exit(EXIT_FAILURE);
    }
    
    close(fd);
}

uint8_t* 
create_payload(uint16_t id, size_t payload_size, int randomOrNot) 
{
    if (payload_size < sizeof(id)) 
    {
        printf("Packet size should be at least 2 bytes for the ID.\n");
        return NULL;
    }

    uint8_t* payload = (uint8_t*)calloc(payload_size, sizeof(uint8_t));
    if (payload == NULL) 
    {
        printf("Memory allocation failed.\n");
        return NULL;
    }
    if(randomOrNot == 1) 
    {
        createRandomSeq(payload, payload_size);
    }
    payload[0] = (id >> 8) & 0xFF; // Store the high byte of the ID
    payload[1] = id & 0xFF;        // Store the low byte of the ID

    return payload;
}

void 
sendUdp(ConfigInfo *cfg, int sockfd, struct sockaddr_in *addr, int sizeAddr, int randomOrNot)
{
    for(int i = 0; i < cfg->num_udp_packet; i++) 
    {
        uint8_t* payload = create_payload((uint16_t)i, cfg->size_udp, randomOrNot);
        if (sendto(sockfd, payload, cfg->size_udp, 0, (struct sockaddr*)addr, sizeAddr) < 0)
        {
            printf("Can't send\n");
            exit(1);
        }

        free(payload);
    }
}

struct Cfg_mode 
{
	ConfigInfo *cfg;
	int mode;
};

void 
*send_packets(void *arg) 
{
	struct Cfg_mode *cfgm = arg;
    struct sockaddr_in udp_addr;
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = inet_addr(cfgm->cfg->ip);
    udp_addr.sin_port = htons(cfgm->cfg->dst_port_udp);
    int udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    int ttl = cfgm->cfg->ttl;  
    if (setsockopt(udp_sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    int val = IP_PMTUDISC_DO;
    setsockopt(udp_sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val));
    int sockfd1 = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sockfd1 == -1) 
    {
        perror("Failed to create socket");
        exit(1);
    }
    send_syn_packet(cfgm->cfg, cfgm->cfg->head_syn, sockfd1);
    sendUdp(cfgm->cfg, udp_sockfd, &udp_addr, sizeof(udp_addr), cfgm->mode);
    int sockfd2 = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sockfd2 == -1) 
    {
        perror("Failed to create socket");
        exit(1);
    }
    send_syn_packet(cfgm->cfg, cfgm->cfg->tail_syn, sockfd2);
    close(sockfd1);
    close(sockfd2);
    
}

double
oneRound(ConfigInfo *cfg, int mode) 
{
    pthread_t receive_thread, send_thread;
    struct Cfg_mode cfgm;
    cfgm.cfg = cfg;
    cfgm.mode = mode;

    struct timeval start, end;
    pthread_create(&receive_thread, NULL, receive_rst, (void *)&start);
    pthread_create(&send_thread, NULL, send_packets, (void *)&cfgm);
    pthread_create(&receive_thread, NULL, receive_rst, (void *)&end);

    pthread_join(receive_thread, NULL);
    pthread_join(send_thread, NULL);


    if((start.tv_sec == 0 && start.tv_usec == 0) || (end.tv_sec == 0 && end.tv_usec == 0)) 
    {
        return -1;
    }
    double elapsed_time = (end.tv_sec - start.tv_sec) + ((end.tv_usec - start.tv_usec) / 1000000.0);
    LOG("elapsed time is %lf\n", elapsed_time);
    return elapsed_time;
}


int main(int argc, char *argv[]) 
{
	if(argc < 2) 
    {
		printf("Not enough arguments!");
		return 1;
	}
    char *buffer = read_file("config.json");
    ConfigInfo cfg;
    memset(&cfg, 0, sizeof(cfg));
    parseJSON(buffer, &cfg);

    double res1 = oneRound(&cfg, 0);
    if(res1 < 0) 
    {
        printf("Failed to detect due to insufficient information.");
        return 1;
    }
    double res2 = oneRound(&cfg, 1);
    if(res2 < 0) 
    {
        printf("Failed to detect due to insufficient information.");
        return 1;
    }
	
    double result = res1 - res2;
    if (fabs(result) > 0.1) 
    {
        ("Compression detected!\n");
    } 
    else 
    {
        printf("No compression was detected.\n");
    }
    return 0;
}