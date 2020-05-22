/*
 * This is a plain C implementation of ProgPoW.  This source file is created by
 * merging and revising pieces of (pseudo)code from different files in upstream
 * ProgPoW (README.md, libethash-cl/CLMiner_kernel.cl, libprogpow/ProgPow.h,
 * libethash-cuda/cuda_helper.h) and go-ethereum/consensus/ethash/progpow.go.
 *
 * The uses of C++ references have been removed, but some structs are still
 * passed and returned by value (allowed in C, just uncommon and inefficient).
 *
 * Older gcc needs explicit -std=c99 to compile this.
 */

#include <stddef.h>
#include <stdint.h>

#include "progpow.h"

progPowStats_t *progPowStats = NULL;

/* These are from libethash-cuda/cuda_helper.h */
#define ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define ROTR32(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

/* clz() and popcount() are based on go-ethereum/consensus/ethash/progpow.go */
static uint32_t clz(uint32_t a)
{
    for (uint32_t i = 0; i < 32; i++) {
        if ((a >> (31 - i)) > 0)
            return i;
    }
    return 32;
}

static uint32_t popcount(uint32_t a)
{
    uint32_t count = 0;
    for (uint32_t i = 0; i < 32; i++) {
        if (((a >> (31 - i)) & 1) == 1)
            count++;
    }
    return count;
}

/* mul_hi(), min(), max() are trivial and written from scratch */
static uint32_t mul_hi(uint32_t a, uint32_t b)
{
    return ((uint64_t)a * b) >> 32;
}

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

/* bswap() is written from scratch using approach seen e.g. in JtR */
static uint32_t bswap(uint32_t a)
{
    a = (a << 16) | (a >> 16);
    return ((a & 0x00ff00ff) << 8) | ((a >> 8) & 0x00ff00ff);
}

/* keccak* are from libethash-cl/CLMiner_kernel.cl */

// Implementation based on:
// https://github.com/mjosaarinen/tiny_sha3/blob/master/sha3.c

static const uint32_t keccakf_rndc[22] = {
    0x00000001, 0x00008082, 0x0000808a, 0x80008000, 0x0000808b, 0x80000001,
    0x80008081, 0x00008009, 0x0000008a, 0x00000088, 0x80008009, 0x8000000a,
    0x8000808b, 0x0000008b, 0x00008089, 0x00008003, 0x00008002, 0x00000080,
    0x0000800a, 0x8000000a, 0x80008081, 0x00008080
};

// Implementation of the Keccakf transformation with a width of 800
static void keccak_f800_round(uint32_t st[25], const int r)
{

    const uint32_t keccakf_rotc[24] = {
        1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
        27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44
    };
    const uint32_t keccakf_piln[24] = {
        10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4,
        15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1
    };

    uint32_t t, bc[5];
    // Theta
    for (int i = 0; i < 5; i++)
        bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];

    for (int i = 0; i < 5; i++) {
        t = bc[(i + 4) % 5] ^ ROTL32(bc[(i + 1) % 5], 1u);
        for (uint32_t j = 0; j < 25; j += 5)
            st[j + i] ^= t;
    }

    // Rho Pi
    t = st[1];
    for (int i = 0; i < 24; i++) {
        uint32_t j = keccakf_piln[i];
        bc[0] = st[j];
        st[j] = ROTL32(t, keccakf_rotc[i]);
        t = bc[0];
    }

    //  Chi
    for (uint32_t j = 0; j < 25; j += 5) {
        for (int i = 0; i < 5; i++)
            bc[i] = st[j + i];
        for (int i = 0; i < 5; i++)
            st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
    }

    //  Iota
    st[0] ^= keccakf_rndc[r];
}

/* Most of the below is based on pieces from upstream ProgPoW README.md */
static hash32_t keccak_f800_progpow(hash32_t header, uint64_t seed, hash32_t digest)
{
    uint32_t st[25];

    // Initialization
    for (int i = 0; i < 25; i++)
        st[i] = 0;

    // Absorb phase for fixed 18 words of input
    for (int i = 0; i < 8; i++)
        st[i] = header.uint32s[i];
    st[8] = seed;
    st[9] = seed >> 32;
    for (int i = 0; i < 8; i++)
        st[10+i] = digest.uint32s[i];

    // keccak_f800 call for the single absorb pass
    for (int r = 0; r < 22; r++)
        keccak_f800_round(st, r);

    // Squeeze phase for fixed 8 words of output
    hash32_t ret;
    for (int i = 0; i < 8; i++)
        ret.uint32s[i] = st[i];

    return ret;
}

static const uint32_t FNV_PRIME = 0x1000193;
static const uint32_t FNV_OFFSET_BASIS = 0x811c9dc5;

static uint32_t fnv1a(uint32_t h, uint32_t d)
{
    return (h ^ d) * FNV_PRIME;
}

typedef struct {
    uint32_t z, w, jsr, jcong;
} kiss99_t;

// KISS99 is simple, fast, and passes the TestU01 suite
// https://en.wikipedia.org/wiki/KISS_(algorithm)
// http://www.cse.yorku.ca/~oz/marsaglia-rng.html
static uint32_t kiss99(kiss99_t *st)
{
    st->z = 36969 * (st->z & 65535) + (st->z >> 16);
    st->w = 18000 * (st->w & 65535) + (st->w >> 16);
    uint32_t MWC = ((st->z << 16) + st->w);
    st->jsr ^= (st->jsr << 17);
    st->jsr ^= (st->jsr >> 13);
    st->jsr ^= (st->jsr << 5);
    st->jcong = 69069 * st->jcong + 1234567;
    return ((MWC^st->jcong) + st->jsr);
}

static void fill_mix(
    uint64_t seed,
    uint32_t lane_id,
    uint32_t mix[PROGPOW_REGS]
)
{
    // Use FNV to expand the per-warp seed to per-lane
    // Use KISS to expand the per-lane seed to fill mix
    kiss99_t st;
    st.z = fnv1a(FNV_OFFSET_BASIS, seed);
    st.w = fnv1a(st.z, seed >> 32);
    st.jsr = fnv1a(st.w, lane_id);
    st.jcong = fnv1a(st.jsr, lane_id);
    for (int i = 0; i < PROGPOW_REGS; i++)
        mix[i] = kiss99(&st);
}

static void progPowInit(kiss99_t *prog_rnd, uint64_t prog_seed, int mix_seq_dst[PROGPOW_REGS], int mix_seq_src[PROGPOW_REGS])
{
    prog_rnd->z = fnv1a(FNV_OFFSET_BASIS, prog_seed);
    prog_rnd->w = fnv1a(prog_rnd->z, prog_seed >> 32);
    prog_rnd->jsr = fnv1a(prog_rnd->w, prog_seed);
    prog_rnd->jcong = fnv1a(prog_rnd->jsr, prog_seed >> 32);
    // Create a random sequence of mix destinations for merge() and mix sources for cache reads
    // guarantees every destination merged once
    // guarantees no duplicate cache reads, which could be optimized away
    // Uses Fisher-Yates shuffle
    for (int i = 0; i < PROGPOW_REGS; i++)
    {
        mix_seq_dst[i] = i;
        mix_seq_src[i] = i;
    }
    for (int i = PROGPOW_REGS - 1; i > 0; i--)
    {
/* Based on swap() function from libprogpow/ProgPow.cpp */
#define swap(a, b) \
{ \
    int t = a; \
    a = b; \
    b = t; \
}
        int j;
        j = kiss99(prog_rnd) % (i + 1);
        swap(mix_seq_dst[i], mix_seq_dst[j]);
        j = kiss99(prog_rnd) % (i + 1);
        swap(mix_seq_src[i], mix_seq_src[j]);
#undef swap
    }
}

// Merge new data from b into the value in a
// Assuming A has high entropy only do ops that retain entropy
// even if B is low entropy
// (IE don't do A&B)
static uint32_t merge(uint32_t a, uint32_t b, uint32_t r)
{
    if (progPowStats)
    {
        progPowStats->merge_total++;
        progPowStats->merge[r % 4]++;
    }
    switch (r % 4)
    {
    default: // pacify the compiler
    case 0: return (a * 33) + b;
    case 1: return (a ^ b) * 33;
    // prevent rotate by 0 which is a NOP
    case 2: return ROTL32(a, ((r >> 16) % 31) + 1) ^ b;
    case 3: return ROTR32(a, ((r >> 16) % 31) + 1) ^ b;
    }
}

// Random math between two input values
static uint32_t math(uint32_t a, uint32_t b, uint32_t r)
{
    if (progPowStats)
    {
        progPowStats->math_total++;
        progPowStats->math[r % 11]++;
    }
    switch (r % 11)
    {
    default: // pacify the compiler
    case 0: return a + b;
    case 1: return a * b;
    case 2: return mul_hi(a, b);
    case 3: return min(a, b);
    case 4: return ROTL32(a, b);
    case 5: return ROTR32(a, b);
    case 6: return a & b;
    case 7: return a | b;
    case 8: return a ^ b;
    case 9: return clz(a) + clz(b);
    case 10: return popcount(a) + popcount(b);
    }
}

static void progPowLoop(
    const uint64_t prog_seed,
    const uint32_t loop,
    uint32_t mix[PROGPOW_LANES][PROGPOW_REGS],
    const uint32_t *dag,
    const uint64_t dag_bytes)
{
    if (progPowStats)
        progPowStats->dag_loads++;
    // dag_entry holds the 256 bytes of data loaded from the DAG
    uint32_t dag_entry[PROGPOW_LANES][PROGPOW_DAG_LOADS];
    // On each loop iteration rotate which lane is the source of the DAG address.
    // The source lane's mix[0] value is used to ensure the last loop's DAG data feeds into this loop's address.
    // dag_addr_base is which 256-byte entry within the DAG will be accessed
    uint32_t dag_addr_base = mix[loop%PROGPOW_LANES][0] %
        (dag_bytes / (PROGPOW_LANES*PROGPOW_DAG_LOADS*sizeof(uint32_t)));
    for (int l = 0; l < PROGPOW_LANES; l++)
    {
        // Lanes access DAG_LOADS sequential words from the dag entry
        // Shuffle which portion of the entry each lane accesses each iteration by XORing lane and loop.
        // This prevents multi-chip ASICs from each storing just a portion of the DAG
        size_t dag_addr_lane = dag_addr_base * PROGPOW_LANES + (l ^ loop) % PROGPOW_LANES;
        for (int i = 0; i < PROGPOW_DAG_LOADS; i++)
        {
            if (progPowStats)
                progPowStats->dag_load_bytes += sizeof(*dag);
            dag_entry[l][i] = dag[dag_addr_lane * PROGPOW_DAG_LOADS + i];
        }
    }

    // Initialize the program seed and sequences
    // When mining these are evaluated on the CPU and compiled away
    int mix_seq_dst[PROGPOW_REGS];
    int mix_seq_src[PROGPOW_REGS];
    int mix_seq_dst_cnt = 0;
    int mix_seq_src_cnt = 0;

    kiss99_t prog_rnd;
    progPowInit(&prog_rnd, prog_seed, mix_seq_dst, mix_seq_src);

    int max_i = max(PROGPOW_CNT_CACHE, PROGPOW_CNT_MATH);
    for (int i = 0; i < max_i; i++)
    {
        if (i < PROGPOW_CNT_CACHE)
        {
            // Cached memory access
            // lanes access random 32-bit locations within the first portion of the DAG
            int src = mix_seq_src[(mix_seq_src_cnt++)%PROGPOW_REGS];
            int dst = mix_seq_dst[(mix_seq_dst_cnt++)%PROGPOW_REGS];
            int sel = kiss99(&prog_rnd);
            for (int l = 0; l < PROGPOW_LANES; l++)
            {
                if (progPowStats)
                {
                    progPowStats->cache_loads++;
                    progPowStats->cache_load_bytes += sizeof(*dag);
                }
                uint32_t offset = mix[l][src] % (PROGPOW_CACHE_BYTES/sizeof(uint32_t));
                mix[l][dst] = merge(mix[l][dst], dag[offset], sel);
            }
        }
        if (i < PROGPOW_CNT_MATH)
        {
            // Random Math
            // Generate 2 unique sources
            int src_rnd = kiss99(&prog_rnd) % (PROGPOW_REGS * (PROGPOW_REGS-1));
            int src1 = src_rnd % PROGPOW_REGS; // 0 <= src1 < PROGPOW_REGS
            int src2 = src_rnd / PROGPOW_REGS; // 0 <= src2 < PROGPOW_REGS - 1
            if (src2 >= src1) ++src2; // src2 is now any reg other than src1
            int sel1 = kiss99(&prog_rnd);
            int dst  = mix_seq_dst[(mix_seq_dst_cnt++)%PROGPOW_REGS];
            int sel2 = kiss99(&prog_rnd);
            for (int l = 0; l < PROGPOW_LANES; l++)
            {
                uint32_t data = math(mix[l][src1], mix[l][src2], sel1);
                mix[l][dst] = merge(mix[l][dst], data, sel2);
            }
        }
    }
    // Consume the global load data at the very end of the loop to allow full latency hiding
    // Always merge into mix[0] to feed the offset calculation
    for (int i = 0; i < PROGPOW_DAG_LOADS; i++)
    {
        int dst = (i==0) ? 0 : mix_seq_dst[(mix_seq_dst_cnt++)%PROGPOW_REGS];
        int sel = kiss99(&prog_rnd);
        for (int l = 0; l < PROGPOW_LANES; l++)
            mix[l][dst] = merge(mix[l][dst], dag_entry[l][i], sel);
    }
}

hash32_t progPowHash(
    const uint64_t prog_seed, // value is (block_number/PROGPOW_PERIOD)
    const uint64_t nonce,
    const hash32_t header,
    const uint32_t *dag, // gigabyte DAG located in framebuffer - the first portion gets cached
    const uint64_t dag_bytes,
    hash32_t *digest_buf)
{
    uint32_t mix[PROGPOW_LANES][PROGPOW_REGS];
    hash32_t digest;
    for (int i = 0; i < 8; i++)
        digest.uint32s[i] = 0;

    // keccak(header..nonce)
    hash32_t seed_256 = keccak_f800_progpow(header, nonce, digest);
    // endian swap so byte 0 of the hash is the MSB of the value
    uint64_t seed = ((uint64_t)bswap(seed_256.uint32s[0]) << 32) | bswap(seed_256.uint32s[1]);

    // initialize mix for all lanes
    for (int l = 0; l < PROGPOW_LANES; l++)
        fill_mix(seed, l, mix[l]);

    // execute the randomly generated inner loop
    for (int i = 0; i < PROGPOW_CNT_DAG; i++)
        progPowLoop(prog_seed, i, mix, dag, dag_bytes);

    // Reduce mix data to a per-lane 32-bit digest
    uint32_t digest_lane[PROGPOW_LANES];
    for (int l = 0; l < PROGPOW_LANES; l++)
    {
        digest_lane[l] = FNV_OFFSET_BASIS;
        for (int i = 0; i < PROGPOW_REGS; i++)
            digest_lane[l] = fnv1a(digest_lane[l], mix[l][i]);
    }
    // Reduce all lanes to a single 256-bit digest
    for (int i = 0; i < 8; i++)
        digest.uint32s[i] = FNV_OFFSET_BASIS;
    for (int l = 0; l < PROGPOW_LANES; l++)
        digest.uint32s[l%8] = fnv1a(digest.uint32s[l%8], digest_lane[l]);

    if (digest_buf)
        *digest_buf = digest;

    // keccak(header .. keccak(header..nonce) .. digest);
    return keccak_f800_progpow(header, seed, digest);
}
