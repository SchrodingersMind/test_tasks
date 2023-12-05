#ifndef DNSSERVER_UTILS
#define DNSSERVER_UTILS

#include <string.h>
#include <ctype.h>

static char* 
lstrip(char* p) {
    while (isspace(*p)) {
        p++;
    }
    return p;
}
static char* 
rstrip(char* p) {
    char* e = p+strlen(p);
    while (isspace(*--e)) {
        *e = 0;
    }
    return p;
}
static char* 
strip(char* target) {
    target = lstrip(target);
    return rstrip(target);
}
static int 
count(const char* haystack, char needle) {
    int res = 0;
    while (*haystack) {
        if (*++haystack == needle) {
            res++;
        }
    }
    return res;
}
static void 
split(char* target, char delim, const char** out) {
    char* tmp = target;
    int c = 0;
    char* tmp2;
    while ((tmp2 = strchr(tmp, delim)) != NULL) {
        *tmp2 = 0;
        out[c] = strdup(strip(tmp));
        c++;
        tmp = tmp2+1;
    }
    out[c] = strdup(strip(tmp));
}

static bool
is_suffix(const char* haystack, const char* needle) {
    int hs_len = strlen(haystack), nd_len = strlen(needle);
    if (hs_len < nd_len)
        return false;

    return !strcmp(&haystack[hs_len-nd_len], needle);
}
#endif //DNSSERVER_PARSE_CONFIG