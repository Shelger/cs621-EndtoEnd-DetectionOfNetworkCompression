#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#define MAX_JSON_SIZE 1024

typedef struct {
    char *ip;
    int src_port_udp, dst_port_udp;
    int head_syn, tail_syn;
    int port_tcp;
    int size_udp;
    int inter_time;
    int num_udp_packet;
    int ttl;
} ConfigInfo;


// Helper function to read a file into a buffer
char* read_file(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    char* buffer = (char*)malloc(size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Error allocating memory: %s\n", strerror(errno));
        fclose(fp);
        return NULL;
    }
    fread(buffer, size, 1, fp);
    fclose(fp);
    buffer[size] = '\0';
    return buffer;
}

// Helper function to skip whitespace characters
char* skip_whitespace(char* str) {
    while (isspace(*str)) {
        str++;
    }
    return str;
}

// int parseJSON(const char* json, ConfigInfo* cfg) {
//     char* str = (char*)json;
//     str = skip_whitespace(str);
//     if (*str != '{') {
//         return -1;
//     }
//     str++;

//     while (*str != '\0' && *str != '}') {
//         char* key = strtok(str, "{}\"\t\n,:");
//         str = NULL;
//         if (key == NULL) {
//             return -1;
//         }

//         char* value = strtok(NULL, "{}\"\t\n,:");
//         if (value == NULL) {
//             return -1;
//         }
//         str = skip_whitespace(strtok(NULL, "{}\"\t\n,:"));
//         if (str == NULL) {
//             return -1;
//         }
//         printf("key: %s\n", key);
//         printf("value: %s\n", value);
//         printf("str: %s\n", str);

//         if (strcmp(key, "ip") == 0) {
//             cfg->ip = strdup(value);
//         } else if (strcmp(key, "src_port_udp") == 0) {
//             cfg->src_port_udp = strdup(value);
//         } else if (strcmp(key, "dst_port_udp") == 0) {
//             cfg->dst_port_udp = strdup(value);
//         } else if (strcmp(key, "head_syn") == 0) {
//             cfg->head_syn = strdup(value);
//         } else if (strcmp(key, "tail_syn") == 0) {
//             cfg->tail_syn = strdup(value);
//         } else if (strcmp(key, "port_tcp") == 0) {
//             cfg->port_tcp = strdup(value);
//         } else if (strcmp(key, "size_udp") == 0) {
//             cfg->size_udp = atoi(value);
//         } else if (strcmp(key, "inter_time") == 0) {
//             cfg->inter_time = atoi(value);
//         } else if (strcmp(key, "num_udp_packet") == 0) {
//             cfg->num_udp_packet = atoi(value);
//         } else if (strcmp(key, "ttl") == 0) {
//             cfg->num_udp_packet = atoi(value);
//         }
//     }
//     return 0;
// }
void
parseJSON(char *input, ConfigInfo* config) 
{
    char *token = strtok(input, "{}\"\t\n,: ");
    char *value;
    while (token != NULL) {
        value = strtok(NULL, "{}\"\t\n,: ");
        if(value == NULL) break;
        // printf("key: %s\n", token);
        // printf("value: %s\n", value);
        if (strcmp(token, "ip") == 0) {
            config->ip = value;
        } else if (strcmp(token, "src_port_udp") == 0) {
            config->src_port_udp = atoi(value);
        } else if (strcmp(token, "dst_port_udp") == 0) {
            config->dst_port_udp = atoi(value);
        } else if (strcmp(token, "head_syn") == 0) {
            config->head_syn = atoi(value);
        } else if (strcmp(token, "tail_syn") == 0) {
            config->tail_syn = atoi(value);
        } else if (strcmp(token, "port_tcp") == 0) {
            config->port_tcp = atoi(value);
        } else if (strcmp(token, "size_udp") == 0) {
            config->size_udp = atoi(value);
        } else if (strcmp(token, "inter_time") == 0) {
            config->inter_time = atoi(value);
        } else if (strcmp(token, "num_udp_packet") == 0) {
            config->num_udp_packet = atoi(value);
        } else if (strcmp(token, "ttl") == 0){
            config->ttl = atoi(value);
        }
        token = strtok(NULL, "{}\"\t\n,: ");
    }
}