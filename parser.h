#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct configInfo {
    char *ip;
    char *src_port_udp, *dst_port_udp;
    char *head_syn, *tail_syn;
    char *port_tcp;
    int size_udp;
    int inter_time;
    int num_udp_packet;
    int ttl;
};

void parseJSON(char *input, struct configInfo* config) {
    char *copy = malloc(1024);
    strcpy(copy, input);
    char *token = strtok(copy, "{}\"\t\n,: ");
    char *value;
    while (token != NULL) {
        value = strtok(NULL, "{}\"\t\n,: ");
        if(value == NULL) break;
        if (strstr(token, "ip")) {
            config->ip = value;
        } else if (strstr(token, "src_port_udp")) {
            config->src_port_udp = value;
        } else if (strstr(token, "dst_port_udp")) {
            config->dst_port_udp = value;
        } else if (strstr(token, "head_syn")) {
            config->head_syn = value;
        } else if (strstr(token, "tail_syn")) {
            config->tail_syn = value;
        } else if (strstr(token, "port_tcp")) {
            config->port_tcp = value;
        } else if (strstr(token, "size_udp")) {
            config->size_udp = atoi(value);
        } else if (strstr(token, "inter_time")) {
            config->inter_time = atoi(value);
        } else if (strstr(token, "num_udp_packet")) {
            config->num_udp_packet = atoi(value);
        } else if (strstr(token, "ttl")){
            config->ttl = atoi(value);
        }
        token = strtok(NULL, "{}\"\t\n,: ");
    }
    free(copy);
}

// int main(){
//     struct configInfo* config;
//     char jsonString[] = "{\"ip\": \"104.52.6.177\",\"src_port_udp\": 8081, \"dst_port_udp\": 8082}";
//     parseJSON(jsonString, config);
//     // printf("%s\n", jsonString);
//     return 0;
// }