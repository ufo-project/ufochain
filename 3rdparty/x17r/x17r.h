#ifndef X17R_H__
#define X17R_H__

#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>

#include <stddef.h>
#include "sph_types.h"

static void getAlgoString(const uint8_t* prevblock, char *output);

void x17r_hash(void* output, const void* input, const int in_len);

#ifdef __cplusplus
}
#endif

#endif
