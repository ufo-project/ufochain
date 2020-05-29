
#include <stdio.h>
#include <assert.h>
#include <string>

#include "3rdparty/libethash/progpow.hpp"
#include "3rdparty/libdevcore/CommonData.h"
#include "external_progpow.h"


#define ETHASH_EPOCH_LENGTH 30000


bool progpow_hash(uint32_t block_number, const std::string& in, uint64_t nonce, std::string& out)
{
    assert(in.length() == 64);

    const auto& context = progpow::get_global_epoch_context_full(block_number / ETHASH_EPOCH_LENGTH);
    auto header = progpow::hash256_from_bytes(
        dev::fromHex(in).data());

    auto result = progpow::hash(context, block_number, header, nonce);

    out = dev::toHex(result.final_hash.bytes);
}