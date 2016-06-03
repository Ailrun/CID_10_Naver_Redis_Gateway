#ifndef GATEWAY_H_
#define GATEWAY_H_

#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "ae.h"

#define GATEWAY_DEFAULT_PORT 10001

#define GATEWAY_BINDADDR_MAX 16

#define GATEWAY_MAX_CLIENT 1000
#define GATEWAY_DATA_SERVER_NUMBER 4
#define GATEWAY_PARITY_SERVER_NUMBER 2

struct redisGateway
{
    pid_t pid;
    aeEventLoop el;

    int port;
    char *bindaddr[GATEWAY_BINDADDR_MAX];
    int bindaddr_count;
    int ipfd[GATEWAY_BINDADDR_MAX];
    int ipfd_count;

    int max_client;
    int data_server_number;
    int parity_server_number;
};

#endif /* #ifndef GATEWAY_H_ */
