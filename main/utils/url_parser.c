#include "url_parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Static storage: Persistent in RAM, private to this file
static url_data_t static_url_info = {0};

void parse_url(const char* url, char* protocol, char* host, char* pathname, int* port) {
    if (!url) return;

    // 1. Extract Protocol
    const char* proto_end = strstr(url, "://");
    if (proto_end) {
        size_t len = proto_end - url;
        strncpy(protocol, url, (len < 9) ? len : 9);
        protocol[(len < 9) ? len : 9] = '\0';
        url = proto_end + 3;
    } else {
        strcpy(protocol, "http");
    }

    // 2. Markers
    const char* path_start = strchr(url, '/');
    const char* port_start = strchr(url, ':');

    // 3. Host & Port
    if (port_start && (!path_start || port_start < path_start)) {
        size_t host_len = port_start - url;
        strncpy(host, url, (host_len < 63) ? host_len : 63);
        host[(host_len < 63) ? host_len : 63] = '\0';
        *port = atoi(port_start + 1);
    } else {
        size_t host_len = path_start ? (path_start - url) : strlen(url);
        strncpy(host, url, (host_len < 63) ? host_len : 63);
        host[(host_len < 63) ? host_len : 63] = '\0';
        *port = (strcmp(protocol, "https") == 0) ? 443 : 80;
    }

    // 4. Path
    if (path_start) {
        strncpy(pathname, path_start, 127);
        pathname[127] = '\0';
    } else {
        strcpy(pathname, "/");
    }
}

void parse_url_to_static(const char* url) {
    // Reuse the logic from parse_url to populate the static struct
    parse_url(url, 
              static_url_info.protocol, 
              static_url_info.host, 
              static_url_info.pathname, 
              &static_url_info.port);
}
void parse_url_info(const char* url, url_data_t* info){
    parse_url(url, 
              info->protocol, 
              info->host, 
              info->pathname, 
              &(info->port));
}

url_data_t* get_url_data(void) {
    return &static_url_info;
}


//
bool is_lan_ip(const char* host) {
    unsigned int o1, o2, o3, o4;
    
    // 1. Check if string matches IPv4 pattern exactly
    // sscanf returns the number of successfully matched items
    if (sscanf(host, "%u.%u.%u.%u", &o1, &o2, &o3, &o4) != 4) {
        return false; // Not a pure IPv4 string
    }

    // 2. Validate octet ranges (0-255)
    if (o1 > 255 || o2 > 255 || o3 > 255 || o4 > 255) {
        return false;
    }

    // 3. Check RFC 1918 Private Ranges
    // 10.0.0.0 - 10.255.255.255
    if (o1 == 10) return true;
    
    // 172.16.0.0 - 172.31.255.255
    if (o1 == 172 && (o2 >= 16 && o2 <= 31)) return true;
    
    // 192.168.0.0 - 192.168.255.255
    if (o1 == 192 && o2 == 168) return true;
    
    // 127.0.0.1 (Loopback/Localhost)
    if (o1 == 127) return true;

    return false; // Public IP or other
}