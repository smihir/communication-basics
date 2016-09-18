#ifndef RPC_CLNT_H_
#define RPC_CLNT_H_

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1500
struct client {
    // buffer used for sending packets and
    // receiving acks, may need separate buffer
    // if more complicated functionality is added
    uint8_t *buffer;
    size_t buffer_sz;

    int sockfd;
    struct sockaddr svc_addr;
    socklen_t addrlen;
    int rcv_timeout;
};

struct client * clnt_create(char *host, char *port, uint32_t timeout_ms);
void clnt_ping(struct client *clnt, size_t ping_len);
void clnt_ping_tput(struct client *clnt, size_t ping_len);
void clnt_ping_latency(struct client *clnt, size_t ping_len, uint64_t num_pkts);
#endif
