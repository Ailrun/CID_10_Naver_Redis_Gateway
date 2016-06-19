#ifndef USEISA_H_
#define USEISA_H_

#include <string.h>

#define NO_INVERT_MATRIX -2

void init_erasure(int mv, int kv);
void encode_erasure(unsigned char *original, size_t original_len, unsigned char **result, size_t *result_frg_len);
void after_encode_erasure(unsigned char **result);
void decode_erasure(unsigned char **data, size_t frg_len, unsigned char **result);
void after_decode_erasure(unsigned char *result);
void deinit(void);

#endif // #ifndef USEISA_H_
