#ifndef RPC_SVC_H_
#define RPC_SVC_H_

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1500
struct service {
    // buffer used for sending packets and
    // receiving acks, may need separate buffer
    // if more complicated functionality is added
    uint8_t *buffer;
    size_t buffer_sz;

    int sockfd;
};

struct service * svc_create(char *port);
void svc_pong(struct service *svc);
#endif
