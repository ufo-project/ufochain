
#include <stdio.h>
#include <assert.h>
#include <string>

#include "3rdparty/progpow/ethash.h"
#include "3rdparty/progpow/progpow.h"

#include "external_progpow.h"


static const int PROGPOW_BLOCK_NUM = 30000;
static ethash_full_t full = nullptr;


static uint32_t bswap(uint32_t a)
{
    a = (a << 16) | (a >> 16);
    return ((a & 0x00ff00ff) << 8) | ((a >> 8) & 0x00ff00ff);
}

static void unhex(hash32_t* dst, const char* src)
{
    const char* p = src;
    uint32_t* q = dst->uint32s;
    uint32_t v = 0;

    while (*p && q <= &dst->uint32s[7]) {
        if (*p >= '0' && *p <= '9')
            v |= *p - '0';
        else if (*p >= 'a' && *p <= 'f')
            v |= *p - ('a' - 10);
        else
            break;
        if (!((++p - src) & 7))
            * q++ = bswap(v);
        v <<= 4;
    }
}

static std::string hex(const hash32_t* src)
{
    const uint32_t* p = src->uint32s;
    std::string res;
    char tmp[64];

    while (p <= &src->uint32s[7]) {
        sprintf(tmp, "%08x", bswap(*p++));
        res += tmp;
    }
    return res;
}

static int ethash_full_new_callback(unsigned int percent)
{
    fprintf(stderr, "Full DAG init %u%%\r", percent);
    return 0;
}

bool progpow_ethash_init()
{
    ethash_light_t light = ethash_light_new(PROGPOW_BLOCK_NUM);
    if (!light) {
        fprintf(stderr, "ethash_light_new() failed\n");
        return false;
    }
    fprintf(stderr, "Light DAG init done\n");

    full = ethash_full_new(light, ethash_full_new_callback);
    if (!full) {
        fprintf(stderr, "ethash_full_new() failed\n");
        return false;
    }
    fprintf(stderr, "Full DAG init done\n");

    printf("ProgPoW version %u.%u.%u\nBlock\t%u\n",
        PROGPOW_VERSION / 100, PROGPOW_VERSION % 100 / 10, PROGPOW_VERSION % 10, PROGPOW_BLOCK_NUM);

    return true;
}

bool progpow_hash(const std::string& in, uint64_t nonce, std::string& out)
{
    assert(in.length() == 64);
    assert(full != nullptr);
    hash32_t header;
    unhex(&header, in.c_str());

    hash32_t digest;
    hash32_t result = progPowHash(PROGPOW_BLOCK_NUM / PROGPOW_PERIOD,
        nonce, header, (const uint32_t*)ethash_full_dag(full), ethash_full_dag_size(full),
        &digest);

    out = hex(&digest);

    return true;
}

