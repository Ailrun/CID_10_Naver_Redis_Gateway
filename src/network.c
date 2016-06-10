#include "gateway.h"

void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask)
{
    int cport;
    char cip[30];
    int cfd = anetTcpAccept(gateway.neterr, fd, cip, sizeof(cip), &cport);

    
}
