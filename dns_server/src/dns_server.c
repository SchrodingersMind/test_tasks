#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ini.h>
#include <pthread.h> 
#include <sys/epoll.h>
#include <sys/queue.h>
#include <ldns/ldns.h>
#include <regex.h>

#include "parse_config.h"

#define CONFIG_FILE "dns.ini"
#define MAX_PKT_SIZE 4096

typedef struct {
    int addrlen;
    struct sockaddr_in client;
    int pkt_len;
    char pkt[MAX_PKT_SIZE];
} req_data;


struct pkts_entry {
    req_data data;
    const configuration *config;
    STAILQ_ENTRY(pkts_entry) entries;
};

STAILQ_HEAD(stailhead, pkts_entry) pkts_head;
pthread_mutex_t pkts_mutex;
int listen_sock;


void
requests_handler() {
    struct timespec ts;
    struct pkts_entry* cur_elem;
    struct sockaddr_in servaddr;
    socklen_t addrlen = sizeof(servaddr);
    int parent_sock, len;
    char tmpbuf[MAX_PKT_SIZE];
    // While waiting, sleep 0.5 second
    int milliseconds = 500;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;

    for (;;) {
        if (STAILQ_EMPTY(&pkts_head)) {
            nanosleep(&ts, NULL);
        } else {
            pthread_mutex_lock(&pkts_mutex);
            cur_elem = STAILQ_FIRST(&pkts_head);
            STAILQ_REMOVE_HEAD(&pkts_head, entries);
            pthread_mutex_unlock(&pkts_mutex);

            // Reroute request to parent server
            parent_sock = socket(AF_INET, SOCK_DGRAM, 0); 
            if (parent_sock == -1) { 
                perror("socket: parent_sock"); 
                continue;
            }
            bzero(&servaddr, sizeof(servaddr)); 
    
            servaddr.sin_family = AF_INET; 
            servaddr.sin_addr.s_addr = cur_elem->config->parent;
            servaddr.sin_port = htons(cur_elem->config->parent_port); 
            
            len = sendto(parent_sock, cur_elem->data.pkt, cur_elem->data.pkt_len, 0, 
                        (struct sockaddr*)&servaddr, sizeof(servaddr));
            if (len < 0) {
                perror("sendto: parent server");
                close(parent_sock);
                continue;
            }
            len = recvfrom(parent_sock, (char *)tmpbuf, sizeof(tmpbuf),  MSG_WAITALL, 
                            (struct sockaddr*) &servaddr, &addrlen);
            if (len < 0) {
                perror("recvfrom: parent server");
                close(parent_sock);
                continue;
            }
            len = sendto(listen_sock, tmpbuf, len, 0, (struct sockaddr*)&cur_elem->data.client, cur_elem->data.addrlen);
            if (len < 0) {
                perror("sendto: client");
            }
            close(parent_sock);
            free(cur_elem);
        }
    }
}

bool 
is_domain_in_blacklist(char* domain, const configuration *config) {
    if (domain[strlen(domain)-1] == '.') {
        domain[strlen(domain)-1] = '\0';
    }
    for (int i=0; i<config->blacklist_cnt; i++) {
        if (!regexec(&config->blacklist[i], domain, 0, NULL, REG_ICASE)) {
            return true;
        }
    }
    return false;
}


void 
handle_input_packet(req_data *pkt, const configuration *config) {
    ldns_status status;
	ldns_pkt *query_pkt;
	ldns_pkt *answer_pkt;
	size_t answer_size;
	ldns_rr *query_rr;
	ldns_rr_list *answer_qr;
	ldns_rr_list *answer_an;
	ldns_rr_list *answer_ns;
	ldns_rr_list *answer_ad;
	ldns_rdf *origin = NULL;
    uint8_t* outbuf;
    char* domain_str;
    struct pkts_entry *entry;

    status = ldns_wire2pkt(&query_pkt, pkt->pkt, (size_t)pkt->pkt_len);
    if (status != LDNS_STATUS_OK) {
        printf("Got bad packet: %s\n", ldns_get_errorstr_by_id(status));
        return;
    }

    query_rr = ldns_rr_list_rr(ldns_pkt_question(query_pkt), 0);
    // ldns_rdf* owner = ldns_rr_owner(query_rr);
    // printf("Requested domain name: %s\n", ldns_rdf2str(owner));
    origin = ldns_rr_owner(query_rr);
    domain_str = ldns_rdf2str(origin);
    if (is_domain_in_blacklist(domain_str, config)) {
        // Domain is blacklisted, return empty response
        answer_qr = ldns_rr_list_new();
        ldns_rr_list_push_rr(answer_qr, ldns_rr_clone(query_rr));

        answer_an = ldns_rr_list_new();
        answer_pkt = ldns_pkt_new();
        answer_ns = ldns_rr_list_new();
        answer_ad = ldns_rr_list_new();
        
        ldns_pkt_set_qr(answer_pkt, 1);
        ldns_pkt_set_aa(answer_pkt, 1);
        ldns_pkt_set_id(answer_pkt, ldns_pkt_id(query_pkt));

        ldns_pkt_push_rr_list(answer_pkt, LDNS_SECTION_QUESTION, answer_qr);
        ldns_pkt_push_rr_list(answer_pkt, LDNS_SECTION_ANSWER, answer_an);
        ldns_pkt_push_rr_list(answer_pkt, LDNS_SECTION_AUTHORITY, answer_ns);
        ldns_pkt_push_rr_list(answer_pkt, LDNS_SECTION_ADDITIONAL, answer_ad);

        status = ldns_pkt2wire(&outbuf, answer_pkt, &answer_size);
        printf("Answer packet size: %u bytes.\n", (unsigned int) answer_size);
        if (status != LDNS_STATUS_OK) {
            printf("ldns_pkt2wire: %s\n", ldns_get_errorstr_by_id(status));
        } else {
            sendto(listen_sock, outbuf, answer_size, 0, (struct sockaddr*)&pkt->client, pkt->addrlen);
        }

        ldns_pkt_free(answer_pkt);
        LDNS_FREE(outbuf);
        ldns_rr_list_free(answer_qr);
        ldns_rr_list_free(answer_an);
        ldns_rr_list_free(answer_ns);
        ldns_rr_list_free(answer_ad);
        // ldns_rdf_deep_free(origin);
    } else {
        // Add to queue (response will be handled in other thread)
        entry = malloc(sizeof(struct pkts_entry));
        memcpy(&entry->data, pkt, sizeof(req_data));
        entry->config = config;
        pthread_mutex_lock(&pkts_mutex);
        STAILQ_INSERT_TAIL(&pkts_head, entry, entries);
        pthread_mutex_unlock(&pkts_mutex);
    }

    LDNS_FREE(domain_str);
    ldns_pkt_free(query_pkt);
}

// Code taken from epoll manpage
void
start_server(const configuration *config) {
    #define MAX_EVENTS 100
    struct epoll_event ev, events[MAX_EVENTS];
    struct sockaddr_in servaddr, cliaddr;
    socklen_t addrlen = sizeof(cliaddr);
    int conn_sock, nfds, epollfd, recvlen;
    char packet_in[MAX_PKT_SIZE];
    req_data cur_pkt;


    listen_sock = socket(AF_INET, SOCK_DGRAM, 0); 
    if (listen_sock == -1) { 
        perror("socket: listen_sock"); 
        exit(EXIT_FAILURE);
    } 
    bzero(&servaddr, sizeof(servaddr)); 
    bzero(&cliaddr, sizeof(cliaddr));
    
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(config->server_port); 
    
    if ((bind(listen_sock, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) { 
        perror("bind"); 
        exit(EXIT_FAILURE); 
    }

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = listen_sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int n = 0; n < nfds; ++n) {
            recvlen = recvfrom(listen_sock, cur_pkt.pkt, MAX_PKT_SIZE, 0, (struct sockaddr*)&cliaddr, &addrlen);
            if (recvlen < 0) {
                perror("recvfrom");
            }
            cur_pkt.addrlen = addrlen;
            memcpy(&cur_pkt.client, &cliaddr, sizeof(cliaddr));
            cur_pkt.pkt_len = recvlen;
            handle_input_packet(&cur_pkt, config);
        }
    }
}


int main(int argc, char* argv[])
{
    configuration *config = malloc(sizeof(configuration));
    char* config_file;

    if (argc >= 2) {
        config_file = argv[1];
    } else {
        config_file = CONFIG_FILE;
    }
    printf("[+] Reading configuration from: %s\n", config_file);

    parse_config(config, config_file);
    if (config->parent == NULL) {
        printf("[-] Unable to read parent address from configuration\n");
        return -1;
    }
    if (config->blacklist_cnt == 0) {
        printf("[ ] No blacklisted domains\n");
    } else {
        printf("[+] Loaded %d domains in blacklist\n", config->blacklist_cnt);
    }

    STAILQ_INIT(&pkts_head);
    pthread_t thread_id; 
    printf("Spinning-up threads...\n");
    pthread_create(&thread_id, NULL, requests_handler, NULL);

    start_server(config);

    // Clean-up
    pthread_join(thread_id, NULL);
    for (int i=0; i<config->blacklist_cnt; i++) {
        regfree(&config->blacklist[i]);
    }
    free(config);

    return 0;
}