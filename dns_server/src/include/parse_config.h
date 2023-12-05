#ifndef DNSSERVER_PARSE_CONFIG
#define DNSSERVER_PARSE_CONFIG

#include <string.h>
#include <ini.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <regex.h>
#include "utils.h"


typedef struct
{
    int server_port;
    int blacklist_cnt;
    in_addr_t parent;
    int parent_port;
    regex_t* blacklist;
} configuration;


bool parse_config(configuration *out, const char* path);


# endif //DNSSERVER_PARSE_CONFIG