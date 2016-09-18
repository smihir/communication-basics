#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <sys/errno.h>

#include "rpc_clnt.h"

void usage() {
    printf("test_clnt -s <hostname> -p <port> -n <number of packets> "
           "-l <len of packet>\n");
    exit(1);
}

int main(int argc, char **argv) {
    unsigned long port = UINT_MAX;
    unsigned long numpkts, pktlen;
    char *port_str;
    char *hostname;
    int ch;
    struct client *clnt;

    if (argc < 9) {
        usage();
    }

    while ((ch = getopt(argc, argv, "s:p:l:n:")) != -1) {
        switch (ch) {
            case 's':
                hostname = strdup(optarg);
                break;
            case 'p':
                port = strtoul(optarg, NULL, 10);
                if (port <= 1024 || port > 65536) {
                    printf("Invalid Port\n");
                    usage();
                }
                port_str=strdup(optarg);
                break;
            case 'n':
                numpkts = strtoul(optarg, NULL, 10);
                if (numpkts == 0 && errno == EINVAL) {
                    usage();
                }
                break;
            case 'l':
                pktlen = strtoul(optarg, NULL, 10);
                if (pktlen == 0 && errno == EINVAL) {
                    usage();
                }
                break;
            case '?':
            default:
                usage();
            }
    }

    printf("hostname: %s, port: %lu, numpkts: %lu, "
           "pktlen: %lu\n", hostname, port,
            numpkts, pktlen);
    clnt = clnt_create(hostname, port_str, 100);
    if (clnt == NULL) {
        printf("client is null\n");
        exit(1);
    }
    while(1)
        clnt_ping(clnt, 100);
    return 0;
}
