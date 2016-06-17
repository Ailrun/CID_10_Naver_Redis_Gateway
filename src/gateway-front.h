#ifndef GATEWAY_FRONT_H_

void gateSetCommand(redisClient *c);
void gateGetCommand(redisClient *c);
void gateDelCommand(redisClient *c);

#endif // #ifndef GATEWAY_FRONT_H_
