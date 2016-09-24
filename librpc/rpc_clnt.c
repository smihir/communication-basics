#define _GNU_SOURCE
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
#include <signal.h>
#include <sys/time.h>
#include <sched.h>

#include "rpc_clnt.h"
#include "rpc_log.h"

#define NS_IN_SEC 1000000000UL

int num_sent = 0, num_retries = 0;
int num_send_complete = 0;
int tput = 0, latency = 0;
int sz = 0;
struct timespec time1, time2;

void set_affinity(int cpuid) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpuid, &set);

    if (sched_setaffinity(getpid(), sizeof(set), &set) == -1)
        perror("sched_affinity");
}

uint64_t timediff(struct timespec *time1, struct timespec *time2) {
    uint64_t time1ns = (time1->tv_sec * NS_IN_SEC) + time1->tv_nsec;
    uint64_t time2ns = (time2->tv_sec * NS_IN_SEC) + time2->tv_nsec;

    return time2ns - time1ns;
}

void print_summarystats(struct timespec *tvf, struct timespec *tve,
        unsigned int numpkts_tx, unsigned int numbytes_tx)
{
    double elapsed_msec;

    elapsed_msec = (double)timediff(tvf, tve) / 1000000;

    printf("packets sent: %u \nbytes_sent: %u \n"
            "average packets per second: %f \naverage bytes per second: %f \n"
            "Throughput in Mbps: %f\n"
            "duration (ms): %f \n",
            numpkts_tx, numbytes_tx, numpkts_tx / (elapsed_msec / 1000),
            numbytes_tx / (elapsed_msec / 1000),
            (numbytes_tx / (elapsed_msec / 1000) * 8) / 1000000,
            elapsed_msec);
}

void print_latencystats(struct timespec *tv1, struct timespec *tv2,
        struct timespec *tv3, int retries)
{
    double time_sendto, rtt;

    // convert to time in ms (from ns)
    time_sendto = (double)timediff(tv1, tv2) / 1000000;
    rtt = (double)timediff(tv2, tv3) / 1000000;

    printf("sendto: %f, rtt: %f, retries: %d\n", time_sendto, rtt, retries);
}

void printStats(int signum) {
    printf("packets sent(including retries) %d\n"
           "packets sent(without retries) %d\n"
           "number of retries %d\n", num_sent, num_send_complete,
           num_retries);
    if (tput == 1) {
        clock_gettime(CLOCK_MONOTONIC, &time2);
        print_summarystats(&time1, &time2, num_send_complete,
                num_send_complete * sz);
    }
    exit(0);
}

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

    tv.tv_sec = 0;
    tv.tv_usec = clnt->rcv_timeout * 1000;
    if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("error setting receive timeout");
        goto error_socket;
    }

    clnt->sockfd = socketfd;
    memcpy(&clnt->svc_addr, (const void *)p->ai_addr, p->ai_addrlen);
    clnt->addrlen = p->ai_addrlen;
    signal(SIGINT, printStats);
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

    num_sent++;
    pktlen = recvfrom(clnt->sockfd, clnt->buffer, clnt->buffer_sz, flag,
                &clnt->svc_addr, &clnt->addrlen);
    // recv timeout! resend
    if (pktlen == -1) {
        num_retries++;
        goto send;
    }
    // should be num_sent - num_retries, unless we are interrupted in
    // between
    num_send_complete++;
    return;
}

void clnt_ping_tput(struct client *clnt, size_t ping_len) {
    int flag = 0, ret, pktlen = 0;
    tput = 1;
    sz = ping_len;

    if (clnt == NULL) {
        log("client is null");
        return;
    }

    if (ping_len > clnt->buffer_sz) {
        log("packet len is greater than buffer");
        return;
    }

    clock_gettime(CLOCK_MONOTONIC, &time1);
send:
    ret = sendto(clnt->sockfd, clnt->buffer, ping_len,
                    flag, &clnt->svc_addr, clnt->addrlen);
    if(ret == -1) {
        perror("send error");
        return;
    } else if (ret < ping_len) {
        log("complete packet not sent!");
    }

    num_sent++;
    pktlen = recvfrom(clnt->sockfd, clnt->buffer, clnt->buffer_sz, flag,
                &clnt->svc_addr, &clnt->addrlen);
    // recv timeout! resend
    if (pktlen == -1) {
        num_retries++;
        goto send;
    }
    // should be num_sent - num_retries, unless we are interrupted in
    // between
    num_send_complete++;
    goto send;
    return;
}

void clnt_ping_latency(struct client *clnt, size_t ping_len,
        uint64_t num_pkts) {
    int flag = 0, ret, pktlen = 0, retries = 0;
    latency = 1;
    struct timespec time1, time2, time3;

    if (clnt == NULL) {
        log("client is null");
        return;
    }

    if (ping_len > clnt->buffer_sz) {
        log("packet len is greater than buffer");
        return;
    }

    set_affinity(0);
send:
    if (num_pkts == 0)
        usleep(200000);
    else if (num_pkts == num_send_complete)
        return;

    if (retries == 0) {
        clock_gettime(CLOCK_MONOTONIC, &time1);
        clnt->buffer[0] = (char)rand();
    }
    ret = sendto(clnt->sockfd, clnt->buffer, ping_len,
                    flag, &clnt->svc_addr, clnt->addrlen);
    if (retries == 0)
        clock_gettime(CLOCK_MONOTONIC, &time2);
    if(ret == -1) {
        perror("send error");
        return;
    }

    num_sent++;
    pktlen = recvfrom(clnt->sockfd, clnt->buffer, clnt->buffer_sz, flag,
                &clnt->svc_addr, &clnt->addrlen);
    // recv timeout! resend
    if (pktlen == -1) {
        num_retries++;
        retries++;
        goto send;
    }
    clock_gettime(CLOCK_MONOTONIC, &time3);
    // should be num_sent - num_retries, unless we are interrupted in
    // between
    print_latencystats(&time1, &time2, &time3, retries);
    retries = 0;
    num_send_complete++;

    goto send;
    return;
}

void clnt_udp_tput(struct client *clnt, size_t ping_len) {
    int flag = 0, ret;
    tput = 1;
    sz = ping_len;

    if (clnt == NULL) {
        log("client is null");
        return;
    }

    if (ping_len > clnt->buffer_sz) {
        log("packet len is greater than buffer");
        return;
    }

    clock_gettime(CLOCK_MONOTONIC, &time1);
send:
    ret = sendto(clnt->sockfd, clnt->buffer, ping_len,
                    flag, &clnt->svc_addr, clnt->addrlen);
    if(ret == -1) {
        perror("send error");
        return;
    } else if (ret < ping_len) {
        log("complete packet not sent!");
    }

    num_sent++;
    num_send_complete++;
    goto send;
    return;
}
