#ifndef MURMUR_H
#define MURMUR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void MurmurHash3_x86_128(const void *key, const int len, uint32_t seed, void *out);

#ifdef __cplusplus
}
#endif

#endif // MURMUR_H

