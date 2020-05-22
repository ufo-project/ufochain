#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* These are from libprogpow/ProgPow.h and are used by progPowHash() itself */
// lanes that work together calculating a hash
#define PROGPOW_LANES           16
// uint32 registers per lane
#define PROGPOW_REGS            32
// uint32 loads from the DAG per lane
#define PROGPOW_DAG_LOADS       4
// size of the cached portion of the DAG
#define PROGPOW_CACHE_BYTES     (16*1024)
// DAG accesses, also the number of loops executed
#define PROGPOW_CNT_DAG         64

#ifndef PROGPOW_VERSION
#define PROGPOW_VERSION 92
#endif

#if PROGPOW_VERSION == 93
// random cache accesses per loop
#define PROGPOW_CNT_CACHE       11
// random math instructions per loop
#define PROGPOW_CNT_MATH        18

// blocks before changing the random program, used by progPowHash() callers */
#define PROGPOW_PERIOD          10
#else
// random cache accesses per loop
#define PROGPOW_CNT_CACHE       12
// random math instructions per loop
#define PROGPOW_CNT_MATH        20

// blocks before changing the random program, used by progPowHash() callers */
#define PROGPOW_PERIOD          50
#endif

/* hash32_t is from libethash-cl/CLMiner_kernel.cl */
typedef struct
{
    uint32_t uint32s[32 / sizeof(uint32_t)];
} hash32_t;

hash32_t progPowHash(
    const uint64_t prog_seed, // value is (block_number/PROGPOW_PERIOD)
    const uint64_t nonce,
    const hash32_t header,
    const uint32_t *dag, // gigabyte DAG located in framebuffer - the first portion gets cached
    const uint64_t dag_bytes,
    hash32_t *digest_buf);

typedef struct
{
    unsigned long merge_total;
    unsigned long merge[4];
    unsigned long math_total;
    unsigned long math[11];
    unsigned long dag_loads;
    unsigned long dag_load_bytes;
    unsigned long cache_loads;
    unsigned long cache_load_bytes;
} progPowStats_t;

extern progPowStats_t *progPowStats;

#ifdef __cplusplus
}
#endif
