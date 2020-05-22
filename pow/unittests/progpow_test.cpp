/*
 * Test code for the plain C implementation of ProgPoW along with Ethash's DAG
 * computation and optional on-disk caching.  Expected output is:
 *
 * Digest = 11f19805c58ab46610ff9c719dcf0a5f18fa2f1605798eef770c47219274767d
 *
 * which matches upstream ProgPoW's test-vectors.md.
 *
 * Copyright (c) 2019 Solar Designer <solar at openwall.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 *
 * There's ABSOLUTELY NO WARRANTY, express or implied.
 */

/*
 * Light DAG init done
 * Full DAG init done
 * ProgPoW version 0.9.2
 * Block	30000
 * Digest	11f19805c58ab46610ff9c719dcf0a5f18fa2f1605798eef770c47219274767d
 * Result	5b7ccd472dbefdd95b895cac8ece67ff0deb5a6bd2ecc6e162383d00c3728ece
 * DAG	64 loads, 16384 bytes
 * Cache	12288 loads, 49152 bytes
 * Merge	36864 total (8192 12288 5120 11264)
 * Math	20480 total (2048 4096 2048 1024 2048 1024 4096 1024 1024 2048 0)
 * 
 * Light DAG init done
 * Full DAG init done
 * ProgPoW version 0.9.3
 * Block	30000
 * Digest	6018c151b0f9895ebe44a4ca6ce2829e5ba6ae1a68a4ccd05a67ac01219655c1
 * Result	34d8436444aa5c61761ce0bcce0f11401df2eace77f5c14ba7039b86b5800c08
 * DAG	64 loads, 16384 bytes
 * Cache	11264 loads, 45056 bytes
 * Merge	33792 total (8192 7168 10240 8192)
 * Math	18432 total (0 2048 0 5120 5120 0 0 0 4096 0 2048)
*/


#include <string>

#include "pow/external_progpow.h"


int main(int argc, char* argv[])
{
    progpow_ethash_init();

    std::string in = "ffeeddccbbaa9988776655443322110000112233445566778899aabbccddeeff";
    uint64_t nonce = 0x123456789abcdef0;

    std::string out;    

    progpow_hash(in, nonce, out);

    fprintf(stdout, "%s", out.c_str());

    return 0;
}
