// Copyright 2018 The UFO Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stratum.h"
#include "utility/io/json_serializer.h"
#include "nlohmann/json.hpp"
#include "utility/helpers.h"
#include "utility/logger.h"

using json = nlohmann::json;

namespace ufo::stratum {

static const std::string method_strings[] = {
#define METHOD_STR(_, M, __) std::string(#M),
    STRATUM_METHODS(METHOD_STR)
#undef METHOD_STR
};

std::string get_method_str(Method methodId) {
    if (methodId == 0 || methodId >= METHODS_END) return std::string();
    return method_strings[methodId];
}

Method get_method(const std::string& str) {
    if (str.empty()) return null_method;
    for (int i=0; i<METHODS_END; ++i) {
        if (str == method_strings[i]) return Method(i);
    }
    return null_method;
}

std::string get_result_msg(int code) {
    switch (code) {
#define R_MESSAGE(code, _, message) case code: return message;
    STRATUM_RESULTS(R_MESSAGE)
#undef R_MESSAGE
    default: break;
    }
    return "unknown";
}

namespace {

#define DEF_LABEL(label) static const std::string l_##label (#label)
    DEF_LABEL(jsonrpc);
    DEF_LABEL(id);
    DEF_LABEL(method);
    DEF_LABEL(description);
    DEF_LABEL(code);
    DEF_LABEL(message);
    DEF_LABEL(api_key);
    DEF_LABEL(input);
    DEF_LABEL(prev);
    DEF_LABEL(difficulty);
    DEF_LABEL(nbits);
    DEF_LABEL(nonce);
    DEF_LABEL(height);
    DEF_LABEL(nonceprefix);
    DEF_LABEL(forkheight);
    DEF_LABEL(blockhash);
    DEF_LABEL(miner);
    DEF_LABEL(minertype);
    DEF_LABEL(jobid);
    DEF_LABEL(enonce);
    DEF_LABEL(mixhash);
#undef DEF_LABEL

ResultCode parse_json(const void* buf, size_t bufSize, json& o) {
    if (bufSize == 0) return message_corrupted;
    const char* bufc = (const char*)buf;
    try {
        o = json::parse(bufc, bufc + bufSize);
    } catch (const std::exception& e) {
        LOG_ERROR() << "json parse: " << e.what() << "\n" << std::string(bufc, bufc + (bufSize > 1024 ? 1024 : bufSize));
        return message_corrupted;
    }
    return no_error;
}

void append_base(json& o, const Message& m) {
    o[l_jsonrpc] = "2.0";
    o[l_id] = m.id;
    o[l_method] = m.method_str;
}

ResultCode parse_base(const json& o, Message& m) {
    try {
        m.id = o[l_id];
        if (m.id.empty()) return empty_id;
        m.method_str = o[l_method];
        m.method = get_method(m.method_str);
        if (m.method == 0) return unknown_method;
    } catch (const std::exception& e) {
        LOG_ERROR() << "json parse: " << e.what();
        return message_corrupted;
    }
    return no_error;
}

template <typename M> void parse(const json& o, M& m)
{}

template<> void parse(const json& o, Login& m) {
    m.api_key = o[l_api_key];
}

template<> void parse(const json& o, Job& m) {
    m.prev = o[l_prev];
    m.input = o[l_input];
    m.nbits = o[l_nbits];
    m.height = o[l_height];
}

template<> void parse(const json& o, Solution& m) {
    m.nonce = o[l_nonce];
    m.mixhash = o[l_mixhash];
}

template<> void parse(const json& o, Result& m) {
    m.code = o[l_code];
    m.description = o[l_description];
    m.nonceprefix = o.value(l_nonceprefix, std::string());
}

template<> void parse(const json& o, MiningAuthorize& m) {
    m.miner = o[l_miner];
    m.minertype = o[l_minertype];
}

template<> void parse(const json& o, MiningSubscribe& m) {
    m.minertype = o[l_minertype];
}

template<> void parse(const json& o, MiningSubmit& m) {
    m.minertype = o[l_minertype];
    m.jobid = o[l_jobid];
    m.nonce = o[l_nonce];
    m.mixhash = o[l_mixhash];
}

template<> void parse(const json& o, MiningNotify& m) {
    m.jobid = o[l_jobid];
    m.prev = o[l_prev];
    m.input = o[l_input];
}

template<> void parse(const json& o, MiningSetDifficulty& m) {
    m.difficulty = o[l_difficulty];
}

template<> void parse(const json& o, MiningAuthorizeResult& m) {
    m.code = o[l_code];
}

template<> void parse(const json& o, MiningSubscribeResult& m) {
    m.code = o[l_code];
    m.enonce = o[l_enonce];
}

template<> void parse(const json& o, MiningSubmitResult& m) {
    m.code = o[l_code];
}

} //namespace

template <typename M> ResultCode parse_json_msg(const void* buf, size_t bufSize, M& m) {
    json o;
    ResultCode r = parse_json(buf, bufSize, o);
    if (r != 0) return r;
    r = parse_base(o, m);
    if (r != 0) return r;
    parse(m);
    return no_error;
}

Job::Job(const std::string& _id, const Merkle::Hash& _prev, const Merkle::Hash& _input, const Block::PoW& _pow, Height height) :
    Message(_id, job),
    nbits(_pow.m_Difficulty.nBitsPow),
    height(height)
{
    char buf2[72];
    prev = to_hex(buf2, _prev.m_pData, 32);
    input = to_hex(buf2, _input.m_pData, 32);
}

Solution::Solution(const std::string& _id, const Block::PoW& _pow):
  Message(_id, solution)
{
    char buf[Block::PoW::NonceType::nBytes * 2 + 1];
    nonce = to_hex(buf, _pow.m_Nonce.m_pData, Block::PoW::NonceType::nBytes);
    char buf2[Block::PoW::nMixHashBytes * 2 + 1];
    mixhash = to_hex(buf2, _pow.m_MixHash.data(), Block::PoW::nMixHashBytes);
}

bool Solution::fill_pow(Block::PoW& pow) const {
    bool ok = false;
    std::vector<uint8_t> buf = from_hex(nonce, &ok);
    if (!ok || buf.size() != Block::PoW::NonceType::nBytes) return false;
    memcpy(pow.m_Nonce.m_pData, buf.data(), Block::PoW::NonceType::nBytes);

    std::vector<uint8_t> buf2 = from_hex(mixhash, &ok);
    if (!ok || buf2.size() != Block::PoW::nMixHashBytes) return false;
    memcpy(pow.m_MixHash.data(), buf2.data(), Block::PoW::nMixHashBytes);
    return true;
}


MiningAuthorize::MiningAuthorize(const std::string& _id, const std::string& _minerAddress, const std::string& _workerName) :
    Message(std::move(_id), mining_authorize),
    minertype("cpu")
{
    miner = std::move(_minerAddress + "." + _workerName);
}

MiningSubscribe::MiningSubscribe(const std::string& _id) :
    Message(std::move(_id), mining_subscribe),
    minertype("cpu")
{
}

MiningSubmit::MiningSubmit(const std::string& _id, const std::string& _jobid, const std::string& _nonce, const std::string& _mixhash) :
    Message(std::move(_id), mining_submit),
    minertype("cpu"),
    jobid(std::move(_jobid)),
    nonce(std::move(_nonce)),
    mixhash(std::move(_mixhash))
{
}

MiningNotify::MiningNotify(const std::string& _id, const std::string& _jobid, const std::string& _prev, const std::string& _input, Height height) :
    Message(std::move(_id), mining_notify),
    jobid(std::move(_jobid)),
    prev(std::move(_prev)),
    input(std::move(_input)),
    height(height)
{
}

MiningSetDifficulty::MiningSetDifficulty(const std::string& _id, const std::string& _difficulty) :
    Message(std::move(_id), mining_set_difficulty),
    difficulty(std::move(_difficulty))
{
}

MiningAuthorizeResult::MiningAuthorizeResult(const std::string& _id, ResultCode _code) :
    Message(std::move(_id), mining_authorize_result),
    code(_code)
{
}

MiningSubscribeResult::MiningSubscribeResult(const std::string& _id, ResultCode _code, const std::string& _enonce) :
    Message(std::move(_id), mining_subscribe_result),
    code(_code),
    enonce(std::move(_enonce))
{
}

MiningSubmitResult::MiningSubmitResult(const std::string& _id, ResultCode _code) :
    Message(std::move(_id), mining_submit_result),
    code(_code)
{
}


bool append_json_msg(io::FragmentWriter& packer, const Login& m) {
    json o;
    append_base(o, m);
    o[l_api_key] = m.api_key;
    return serialize_json_msg(packer, o);
}

bool append_json_msg(io::FragmentWriter& packer, const Job& m) {
    json o;
    append_base(o, m);
    o[l_prev] = m.prev;
    o[l_input] = m.input;
    o[l_nbits] = m.nbits;
    o[l_height] = m.height;
    return serialize_json_msg(packer, o);
}

bool append_json_msg(io::FragmentWriter& packer, const Cancel& m) {
    json o;
    append_base(o, m);
    return serialize_json_msg(packer, o);
}


bool append_json_msg(io::FragmentWriter& packer, const Solution& m) {
    json o;
    append_base(o, m);
    o[l_nonce] = m.nonce;
    o[l_mixhash] = m.mixhash;
    return serialize_json_msg(packer, o);
}

bool append_json_msg(io::FragmentWriter& packer, const Result& m) {
    json o;
    append_base(o, m);
    o[l_code] = m.code;
    o[l_description] = m.description;
    if (!m.nonceprefix.empty()) o[l_nonceprefix] = m.nonceprefix;
    if (m.forkheight != MaxHeight) o[l_forkheight] = m.forkheight;
    if (!m.blockhash.empty()) o[l_blockhash] = m.blockhash;
    return serialize_json_msg(packer, o);
}

bool append_json_msg(io::FragmentWriter& packer, const MiningAuthorize& m) {
    json o;
    append_base(o, m);
    o[l_miner] = m.miner;
    o[l_minertype] = m.minertype;
    return serialize_json_msg(packer, o);
}

bool append_json_msg(io::FragmentWriter& packer, const MiningSubscribe& m) {
    json o;
    append_base(o, m);
    o[l_minertype] = m.minertype;
    return serialize_json_msg(packer, o);
}

bool append_json_msg(io::FragmentWriter& packer, const MiningSubmit& m) {
    json o;
    append_base(o, m);
    o[l_minertype] = m.minertype;
    o[l_jobid] = m.jobid;
    o[l_nonce] = m.nonce;
    o[l_mixhash] = m.mixhash;
    return serialize_json_msg(packer, o);
}

bool append_json_msg(io::FragmentWriter& packer, const MiningNotify& m) {
    json o;
    append_base(o, m);
    o[l_jobid] = m.jobid;
    o[l_prev] = m.prev;
    o[l_input] = m.input;
    o[l_height] = m.height;
    return serialize_json_msg(packer, o);
}

bool append_json_msg(io::FragmentWriter& packer, const MiningSetDifficulty& m) {
    json o;
    append_base(o, m);
    o[l_difficulty] = m.difficulty;
    return serialize_json_msg(packer, o);
}

bool append_json_msg(io::FragmentWriter& packer, const MiningAuthorizeResult& m) {
    json o;
    append_base(o, m);
    o[l_code] = m.code;
    return serialize_json_msg(packer, o);
}

bool append_json_msg(io::FragmentWriter& packer, const MiningSubscribeResult& m) {
    json o;
    append_base(o, m);
    o[l_code] = m.code;
    o[l_enonce] = m.enonce;
    return serialize_json_msg(packer, o);
}

bool append_json_msg(io::FragmentWriter& packer, const MiningSubmitResult& m) {
    json o;
    append_base(o, m);
    o[l_code] = m.code;
    return serialize_json_msg(packer, o);
}


#define DEF_PARSE_IMPL(_, __, struct_name) \
    ResultCode parse_json_msg(const void* buf, size_t bufSize, struct_name& m) { \
        json o; \
        ResultCode r = parse_json(buf, bufSize, o); \
        if (r != 0) return r; \
        r = parse_base(o, m); \
        if (r != 0) return r; \
        parse(o, m); \
        return no_error; }

STRATUM_METHODS(DEF_PARSE_IMPL)

#undef DEF_PARSE_IMPL

namespace {

template <typename M> bool parse(const json& o, const Message& base, ParserCallback& callback) {
    M m;
    (Message&)m = base;
    parse<M>(o, m);
    return callback.on_message(m);
}

} //namespace

bool parse_json_msg(const void* buf, size_t bufSize, ParserCallback& callback) {
    json o;
    ResultCode r = parse_json(buf, bufSize, o);
    if (r != 0) return false;
    Message m;
    r = parse_base(o, m);
    if (r != 0) return false;

    try {
        switch (m.method) {
#define DEF_PARSE_IMPL(_, method_name, struct_name) case method_name: { return parse<struct_name>(o, m, callback); }
        STRATUM_METHODS(DEF_PARSE_IMPL)
#undef DEF_PARSE_IMPL
        default:break;
        }
    } catch (const std::exception& e) {
        LOG_ERROR() << "json parse: " << e.what();
    }
    return false;
}

} //namespaces
