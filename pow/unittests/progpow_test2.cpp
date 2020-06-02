
//hash_str_no_nonce: 00000000debc2337f1cac3b17af2eb4896ecf471eb9a3a5c2aa8a3f3f51a000000000000000000003cb122e3db784a1da40ba5c1fe1a6fc4d2eabad1b43b6114de08a37b00aa2e5c
//header(sha256 of hash_str_no_nonce): c07acfc3fa2e92ac9ff331bb04743f7851e9d7a99e05ec7589d4b05d1505ae56
//nonce: 1311768467463790320
//result: 49c1c439e996cb656d69b51556e949e99e88a1e850dd01ebbd29b8f1328474b8
//mixhash: b72f1079a42a23f89b4820747a10fd79c843262562512fb5f95900135ce8bfd5



#include "block_crypt.h"
#include "core/ecc.h"
#include "core/merkle.h"
#include "utility/helpers.h"

#include <string>
#include "openssl/sha.h"

#include "pow/external_progpow.h"


int main(int argc, char* argv[])
{
    std::string prev = "debc2337f1cac3b17af2eb4896ecf471eb9a3a5c2aa8a3f3f51a000000000000";
    std::string input = "3cb122e3db784a1da40ba5c1fe1a6fc4d2eabad1b43b6114de08a37b00aa2e5c";
    uint64_t nonce = 0x123456789abcdef0;

    ufo::Merkle::Hash _prev;
    ufo::Merkle::Hash _input;
    bool ok = false;
    
    std::vector<uint8_t> buf = ufo::from_hex(prev, &ok);
    memcpy(_prev.m_pData, buf.data(), 32);

    std::vector<uint8_t> buf2 = ufo::from_hex(input, &ok);
    memcpy(_input.m_pData, buf2.data(), 32);

    ufo::uintBig_t<8> _nonce = nonce;

    unsigned char pDataIn[80];
    unsigned char pDataOut[32];

    memset(pDataIn, 0x0, 4);
    memcpy(pDataIn + 4, (unsigned char*)_prev.m_pData, _prev.nBytes);
    memset(pDataIn + 36, 0x0, 4);

    memcpy(pDataIn + 40, (unsigned char*)_input.m_pData, _input.nBytes);
    memcpy(pDataIn + 72, (unsigned char*)_nonce.m_pData, _nonce.nBytes);
    
    std::string s = ufo::to_hex(pDataIn, 72);

    std::cout << "hash_str_no_nonce: " << s << std::endl;

    unsigned char out[32];
    unsigned char* p = out;

    SHA256((const unsigned char*)s.c_str(), s.length(), p);
    s = ufo::to_hex(out, sizeof(out));

    std::cout << "header(sha256 of hash_str_no_nonce): " << s << std::endl;

    uint64_t n =
        ((uint64_t)_nonce.m_pData[0] << 56) +
        ((uint64_t)_nonce.m_pData[1] << 48) +
        ((uint64_t)_nonce.m_pData[2] << 40) +
        ((uint64_t)_nonce.m_pData[3] << 32) +
        ((uint64_t)_nonce.m_pData[4] << 24) +
        ((uint64_t)_nonce.m_pData[5] << 16) +
        ((uint64_t)_nonce.m_pData[6] << 8) +
        (uint64_t)_nonce.m_pData[7];

    std::cout << "nonce: " << n << std::endl;
    
    std::string r;
    std::string m;
    progpow_hash(54321, s, n, r, m);

    std::cout << "result: " << r << std::endl;
    std::cout << "mixhash: " << m << std::endl;

    return 0;
}
