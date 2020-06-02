
#include <stdio.h>
#include <assert.h>
#include <string>

#include "3rdparty/libethash/progpow.hpp"
#include "3rdparty/libdevcore/CommonData.h"
#include "external_progpow.h"


#define ETHASH_EPOCH_LENGTH 30000


bool progpow_hash(uint32_t block_number, const std::string& in, uint64_t nonce, std::string& out_result, std::string& out_mixhash)
{
    assert(in.length() == 64);

    const auto& context = progpow::get_global_epoch_context_full(block_number / ETHASH_EPOCH_LENGTH);
    auto header = progpow::hash256_from_bytes(
        dev::fromHex(in).data());

    auto result = progpow::hash(context, block_number, header, nonce);

    out_result = dev::toHex(result.final_hash.bytes);
    out_mixhash = dev::toHex(result.mix_hash.bytes);

    return true;
}


bool progpow_hash_verify(uint32_t block_number, const std::string& in, const std::string mix, uint64_t nonce, const std::string target)
{
    assert(in.length() == 64);
    assert(mix.length() == 64);
    assert(target.length() == 64);

    auto _in = progpow::hash256_from_bytes(dev::fromHex(in).data());
    auto _mix = progpow::hash256_from_bytes(dev::fromHex(mix).data());
    auto _target = progpow::hash256_from_bytes(dev::fromHex(target).data());

    uint32_t epoch = block_number / ETHASH_EPOCH_LENGTH;

    auto& context = progpow::get_global_epoch_context(epoch);

    return progpow::verify(context, block_number, _in, _mix, nonce, _target);
}

