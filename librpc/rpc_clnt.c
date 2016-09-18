#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>

#include "rpc_clnt.h"
#include "rpc_log.h"

struct client * clnt_create(char *host, char *port, uint32_t timeout_ms) {
    struct addrinfo hints, *res = NULL, *p;
    int status;
    char ipstr[INET_ADDRSTRLEN];
    int socketfd;
    uint8_t *buffer;
    int sz = BUFFER_SIZE;
    struct client *clnt;
    struct timeval tv;

    clnt = (struct client *)malloc(sizeof(uint8_t) * sizeof(struct client));
    if (clnt == NULL) {
        log("failed to allocate client");
        goto error_clnt;
    }

    buffer = (uint8_t *)malloc(sizeof(uint8_t) * sz);
    if (buffer == NULL) {
        log("memory allocation failed");
        goto error_buf;
    }

    clnt->buffer = buffer;
    clnt->buffer_sz = sz;
    clnt->rcv_timeout = timeout_ms;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if ((status = getaddrinfo(host, port, &hints, &res)) != 0) {
        printf("getaddrinfo: %s\n", gai_strerror(status));
        log("failed to get addrinfo");
        goto error_addrinfo;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *) p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }
        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("%s: %s\n", ipver, ipstr);
        break;
    }

    socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (socketfd < 0) {
        perror("socket open failed");
        goto error_addrinfo;
    }

    if (setsockopt(socketfd, SOL_SOCKET, SO_SNDBUF, &sz, sizeof(sz)) < 0) {
        perror("failed to set send buffer");
        goto error_socket;
    }

    tv.tv_sec = 0;
    tv.tv_usec = clnt->rcv_timeout * 1000;
    if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("error setting receive timeout");
        goto error_socket;
    }

    clnt->sockfd = socketfd;
    memcpy(&clnt->svc_addr, (const void *)p->ai_addr, p->ai_addrlen);
    clnt->addrlen = p->ai_addrlen;
    freeaddrinfo(res);
    return clnt;

error_socket:
    close(socketfd);
error_addrinfo:
    free(buffer);
    if (res != NULL)
        freeaddrinfo(res);
error_buf:
    free(clnt);
error_clnt:
    return NULL;
}

void clnt_ping(struct client *clnt, size_t ping_len) {
    int flag = 0, ret, pktlen = 0;

    if (clnt == NULL) {
        log("client is null");
        return;
    }

    if (ping_len > clnt->buffer_sz) {
        log("packet len is greater than buffer");
        return;
    }

send:
    ret = sendto(clnt->sockfd, clnt->buffer, ping_len,
                    flag, &clnt->svc_addr, clnt->addrlen);
    if(ret == -1) {
        perror("send error");
        return;
    }

    pktlen = recvfrom(clnt->sockfd, clnt->buffer, clnt->buffer_sz, flag, &clnt->svc_addr,
                &clnt->addrlen);
    // recv timeout! resend
    if (pktlen == -1) {
        goto send;
    }
    return;
}
