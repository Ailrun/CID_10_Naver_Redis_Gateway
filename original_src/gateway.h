#ifndef GATEWAY_H_

#define DATA_SERVER_NUM 4
#define PARITY_SERVER_NUM 2
#define ALL_SERVER_NUM (DATA_SERVER_NUM + PARITY_SERVER_NUM)

void bytesToHuman(char *s, unsigned long long n);

void gateSetCommand(redisClient *c);
void gateGetCommand(redisClient *c);
void gateDelCommand(redisClient *c);

void backendInit(void);
void singleRepl(sds *argv, int argc, int sn, sds *result);
#endif // #ifndef GATEWAY_H_
