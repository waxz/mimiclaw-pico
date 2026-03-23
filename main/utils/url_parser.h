#pragma once
#include <stdbool.h>

typedef struct {
    char protocol[16];
    char host[128];
    char pathname[256];
    int port;
} url_data_t;

// Function to parse a URL into the static storage
void parse_url_to_static(const char* url);
void parse_url_info(const char* url, url_data_t* info);
// Function to get a pointer to the parsed data
url_data_t* get_url_data(void);

void parse_url(const char* url, char*protocol,char *host, char*path, int* port) ;
// Returns true if host is a LAN IP (e.g., 192.168.x.x)
bool is_lan_ip(const char* host);