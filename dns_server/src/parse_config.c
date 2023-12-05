#include "parse_config.h"

#define DELIM ','

static void 
setup_blacklist(const char* input, regex_t *out) {
    char* tmp = input;
    int c = 0;
    char* tmp2;
    while ((tmp2 = strchr(tmp, DELIM)) != NULL) {
        *tmp2 = 0;
        regcomp(&out[c], strip(tmp), 0);
        c++;
        tmp = tmp2+1;
    }
    regcomp(&out[c], strip(tmp), 0);
}


static int 
handler(void* user, const char* section, const char* name,
                   char* value)
{
    configuration* pconfig = (configuration*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("main", "parent")) {
        in_addr_t parent_ip = inet_addr(value);
        if (parent_ip != INADDR_NONE) {
            pconfig->parent = parent_ip;
        }
    } else if (MATCH("main", "parent_port")) {
        pconfig->parent_port = atoi(value);
    } else if (MATCH("main", "port")) {
        pconfig->server_port = atoi(value);
    } else if (MATCH("main", "blacklist")) {
        char* tmp;
        pconfig->blacklist_cnt = count(value, DELIM) + 1;
        pconfig->blacklist = malloc(pconfig->blacklist_cnt*sizeof(*pconfig->blacklist));
        setup_blacklist(value, pconfig->blacklist);
    } else {
        return 0;  /* unknown section/name, error */
    }
    return 1;
}


bool 
parse_config(configuration *out, const char* path) {
    // Default port
    out->server_port = 8080;
	if (ini_parse(path, handler, out) < 0) {
        printf("Can't load %s\n", path);
        return 0;
    }
    return 1;
}