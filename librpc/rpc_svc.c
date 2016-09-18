#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <signal.h>

#include "rpc_svc.h"
#include "rpc_log.h"

int num_dropped = 0, num_recv = 0;

void printStats(int signum) {
    printf("packets received %d, packets dropped %d\n", num_recv, num_dropped);
    exit(0);
}

struct service * svc_create(char *port) {
    struct addrinfo hints, *res = NULL;
    int status;
    int socketfd;
    uint8_t *buffer;
    int sz = BUFFER_SIZE;
    struct service *svc;
    struct sockaddr_in myaddr;

    myaddr.sin_addr.s_addr=INADDR_ANY;
    myaddr.sin_family=AF_INET;
    myaddr.sin_port=htons(atoi(port));

    svc = (struct service *)malloc(sizeof(uint8_t) * sizeof(struct service));
    if (svc == NULL) {
        log("failed to allocate client");
        goto error_svc;
    }

    buffer = (uint8_t *)malloc(sizeof(uint8_t) * sz);
    if (buffer == NULL) {
        log("memory allocation failed");
        goto error_buf;
    }

    svc->buffer = buffer;
    svc->buffer_sz = sz;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if ((status = getaddrinfo(NULL, port, &hints, &res)) != 0) {
        printf("getaddrinfo: %s\n", gai_strerror(status));
        log("failed to get addrinfo");
        goto error_addrinfo;
    }

    socketfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (socketfd < 0) {
        perror("socket open failed");
        goto error_addrinfo;
    }

    if (bind(socketfd,(struct sockaddr *) &myaddr, sizeof(myaddr)) != 0) {
        perror("socket binding failed");
        goto error_socket;
    }
    if (setsockopt(socketfd, SOL_SOCKET, SO_SNDBUF, &sz, sizeof(sz)) < 0) {
        perror("failed to set send buffer");
        goto error_socket;
    }

    svc->sockfd = socketfd;
    freeaddrinfo(res);
    signal(SIGINT, printStats);
    return svc;

error_socket:
    close(socketfd);
error_addrinfo:
    free(buffer);
    if (res != NULL)
        freeaddrinfo(res);
error_buf:
    free(svc);
error_svc:
    return NULL;
}

void svc_pong(struct service *svc) {
    int flag = 0, ret, pktlen = 0;
    struct sockaddr src_addr;
    socklen_t addrlen;

    if (svc == NULL) {
        log("client is null");
        return;
    }
rec:
    pktlen = recvfrom(svc->sockfd, svc->buffer, svc->buffer_sz, flag,
                &src_addr, &addrlen);
    // recv timeout! resend
    if (pktlen == -1) {
        perror("recv error");
        goto rec;
    }
    num_recv++;
    ret = sendto(svc->sockfd, svc->buffer, 0,
                    flag, &src_addr, addrlen);
    if(ret == -1) {
        perror("send error");
    }
    goto rec;

    return;
}

void svc_pong_sim_drops(struct service *svc, uint8_t percent_drop) {
    int flag = 0, ret, pktlen = 0, drop = 0;
    struct sockaddr src_addr;
    socklen_t addrlen;
    uint8_t r;

    if (svc == NULL) {
        log("client is null");
        return;
    }
    if (percent_drop > 100) {
        log("setting \%drop to 100 will drop all packets");
        percent_drop = 100;
    }
    srand(time(NULL));
rec:
    r = (uint8_t)(rand() % 100);
    drop = r < percent_drop ? 1 : 0;

    pktlen = recvfrom(svc->sockfd, svc->buffer, svc->buffer_sz, flag,
                &src_addr, &addrlen);
    // recv timeout! resend
    if (pktlen == -1) {
        perror("recv error");
        goto rec;
    }
    num_recv++;

    if (drop == 0) {
        ret = sendto(svc->sockfd, svc->buffer, 0,
                        flag, &src_addr, addrlen);
        if(ret == -1) {
            perror("send error");
        }
    } else {
        num_dropped++;
    }
    goto rec;

    return;
}
