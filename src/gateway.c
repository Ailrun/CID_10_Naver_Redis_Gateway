/*
 * Standard Headers
 */
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>

/*
 * Redis Headers
 */
#include "ae.h"
#include "anet.h"

/*
 * New Headers
 */
#include "gateway.h"

struct redisGateway gateway;

void initGatewayConfig(void)
{

#if DEBUG > 3
    printf("start:: initGatewayConfig\n");
#endif /* #if DEBUG > 3 */

    gateway.port = GATEWAY_DEFAULT_PORT;
    gateway.bindaddr_count = 0;
    gateway.ipfd_count = 0;

    gateway.max_client = GATEWAY_MAX_CLIENT;
    gateway.data_server_number = GATEWAY_DATA_SERVER_NUMBER;
    gateway.parity_server_number = GATEWAY_PARITY_SERVER_NUMBER;

#if DEBUG > 3
    printf("end:: initGatewayConfig\n");
#endif /* #if DEBUG > 3 */

}

int listenToPort(int port, int *fds, int *count) {
    int j;

    /* Force binding of 0.0.0.0 if no bind address is specified, always
     * entering the loop if j == 0. */
    if (gateway.bindaddr_count == 0) gateway.bindaddr[0] = NULL;
    for (j = 0; j < gateway.bindaddr_count || j == 0; j++) {
        if (gateway.bindaddr[j] == NULL) {
            /* Bind * for both IPv6 and IPv4, we enter here only if
             * server.bindaddr_count == 0. */
            fds[*count] = anetTcp6Server(gateway.neterr,port,NULL,
                gateway.tcp_backlog);

            if (fds[*count] != ANET_ERR) {
                anetNonBlock(NULL,fds[*count]);
                (*count)++;
            }

            fds[*count] = anetTcpServer(gateway.neterr,port,NULL,
                gateway.tcp_backlog);

            if (fds[*count] != ANET_ERR) {
                anetNonBlock(NULL,fds[*count]);
                (*count)++;
            }
            /* Exit the loop if we were able to bind * on IPv4 or IPv6,
             * otherwise fds[*count] will be ANET_ERR and we'll print an
             * error and return to the caller with an error. */
            if (*count) break;
        } else if (strchr(gateway.bindaddr[j],':')) {
            /* Bind IPv6 address. */
            fds[*count] = anetTcp6Server(gateway.neterr,port,gateway.bindaddr[j],
                gateway.tcp_backlog);
        } else {
            /* Bind IPv4 address. */
            fds[*count] = anetTcpServer(gateway.neterr,port,gateway.bindaddr[j],
                gateway.tcp_backlog);
        }

        if (fds[*count] == ANET_ERR) {
	    printf("Creating Server TCP listening socket %s:%d: %s",
		   gateway.bindaddr[j] ? gateway.bindaddr[j] : "*", port, gateway.neterr);
	    return -1;
        }
        anetNonBlock(NULL,fds[*count]);
        (*count)++;
    }
    return 0;
}

void initGateway(void)
{

#if DEBUG > 3
    printf("start:: initGateway\n");
#endif /* #if DEBUG > 3 */

    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    gateway.pid = getpid();

    gateway.el = *aeCreateEventLoop(gateway.max_client);

    if (gateway.port != 0 &&
	listenToPort(gateway.port, gateway.ipfd, &gateway.ipfd_count) == -1)
    {
	exit(1);
    }

    for (int j = 0; j < gateway.ipfd_count; j++)
    {
	if (aeCreateFileEvent(gateway.el, gateway.ipfd[j], AE_READABLE, acceptTcpHandler, NULL) == AE_ERR)
	{
	    printf("ERROR!\n");
	}
    }
    
#if DEBUG > 3
    printf("end:: initGateway\n");
#endif /* #if DEBUG > 3 */

}

int main(void)
{

#if DEBUG > 3
    printf("start:: main\n");
#endif /* #if DEBUG > 3 */

    initGatewayConfig();
    initGateway();

    aeMain(gateway.el);

#if DEBUG > 3
    printf("end:: main\n");
#endif /* #if DEBUG > 3 */

}
