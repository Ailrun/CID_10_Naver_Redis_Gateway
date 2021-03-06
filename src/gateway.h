#ifndef GATEWAY_H_

#define RFN_METHOD 0
#define EC_METHOD 1
#define SAVE_METHOD EC_METHOD

#define DATA_SERVER_NUM 4
#define PARITY_SERVER_NUM 2
#define ALL_SERVER_NUM (DATA_SERVER_NUM + PARITY_SERVER_NUM)

#define LOCALHOST ("127.0.0.1")

static struct {
  char *hostip;
  int port;
} allServerConfigs[ALL_SERVER_NUM] = {
  {LOCALHOST, 6380},
  {LOCALHOST, 6381},
  {LOCALHOST, 6382},
  {LOCALHOST, 6383},
  {LOCALHOST, 6384},
  {LOCALHOST, 6385},
  //  {LOCALHOST, 6386},
  //  {LOCALHOST, 6387}
};

void bytesToHuman(char *s, unsigned long long n);

void backendInit(void);
void resetDeadServer(void);
int getDeadServer(void);
int singleRepl(sds *argv, int argc, int sn, sds *result);
#endif // #ifndef GATEWAY_H_
