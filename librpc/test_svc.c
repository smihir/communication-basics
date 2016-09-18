#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <sys/errno.h>

#include "rpc_svc.h"

void usage() {
    printf("test_svc -p <port> [-d <drop_percent>]\n");
    exit(1);
}

int main(int argc, char **argv) {
    unsigned long port = UINT_MAX;
    char *port_str;
    int ch;
    struct service *svc;
    unsigned long drop_percent = 0;

    if (argc < 3) {
        usage();
    }

    while ((ch = getopt(argc, argv, "d:p:")) != -1) {
        switch (ch) {
            case 'd':
                drop_percent = strtoul(optarg, NULL, 10);
                if (drop_percent > 100) {
                    printf("Invalid percent should be between 0 - 100\n");
                    usage();
                }
                break;
            case 'p':
                port = strtoul(optarg, NULL, 10);
                if (port <= 1024 || port > 65536) {
                    printf("Invalid Port\n");
                    usage();
                }
                port_str=strdup(optarg);
                break;
            case '?':
            default:
                usage();
            }
    }

    svc = svc_create(port_str);
    if (svc == NULL) {
        printf("service is null\n");
        exit(1);
    }
    if (drop_percent == 0)
        svc_pong(svc);
    else
        svc_pong_sim_drops(svc, (uint8_t)drop_percent);

    return 0;
}
