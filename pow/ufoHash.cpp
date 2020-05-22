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

#include "core/block_crypt.h"
#include "crypto/equihashR.h"
#include "uint256.h"
#include "arith_uint256.h"
#include <utility>
#include "utility/logger.h"
#include "utility/helpers.h"
#include <mutex>
#include "x17r/x17r.h"
#include "pow/external_progpow.h"

namespace ufo
{

    struct Block::PoW::Helper
    {
	    //blake2b_state m_Blake;

        //EquihashR<N_UFO,K_UFO,0> UfoHashI;
        //EquihashR<N_UFO, K_UFO,3> UfoHashII;
        /*
        PoWScheme* getCurrentPoW(Height h) {
	        if (h < Rules::get().pForks[1].m_Height) {
		        return &UfoHashI;
	        } else {
		        return &UfoHashII;
	        }
        }

        void Reset(const void* pInput, uint32_t nSizeInput, const NonceType& nonce, Height h)
        {
	        getCurrentPoW(h)->InitialiseState(m_Blake);

	        // H(I||...
	        blake2b_update(&m_Blake, (uint8_t*) pInput, nSizeInput);
	        blake2b_update(&m_Blake, nonce.m_pData, nonce.nBytes);
        }

        bool TestDifficulty(const uint8_t* pSol, uint32_t nSol, Difficulty d) const
        {
	        ECC::Hash::Value hv;
	        ECC::Hash::Processor() << Blob(pSol, nSol) >> hv;

	        return d.IsTargetReached(hv);
        }*/
        bool TestDifficulty(const uint8_t* pDigest, uint32_t nDigest, Difficulty d) const
        {
            std::vector<unsigned char> vch;
            vch.resize(nDigest);

            //for (int i = 0; i < nDigest; ++i)
            //	vch[i] = *(pDigest + i);

            std::copy(pDigest, pDigest + nDigest, vch.data());

            uint256 v = uint256(vch);

            return d.IsTargetReached(v);
        }
    };

    bool Block::PoW::Solve(const void* pPrev, uint32_t nSizePrev, const void* pInput, uint32_t nSizeInput, Height height, const Cancel& fnCancel)
    {
        Helper hlp;

        /*
        std::function<bool(const ufo::ByteBuffer&)> fnValid = [this, &hlp](const ufo::ByteBuffer& solution)
        {
            if (!hlp.TestDifficulty(&solution.front(), (uint32_t) solution.size(), m_Difficulty))
            return false;
            assert(solution.size() == m_Indices.size());
                std::copy(solution.begin(), solution.end(), m_Indices.begin());
                return true;
            };
        */

        /*
        std::function<bool(EhSolverCancelCheck)> fnCancelInternal = [fnCancel](EhSolverCancelCheck pos) {
            return fnCancel(false);
        };
        */

        assert(nSizePrev == 32);
        assert(nSizeInput == 32);
        //Merkle::Hash hv;
        //memcpy(hv.m_pData, (unsigned char*)pInput, 32);

        unsigned char pDataIn[80];
        unsigned char pDataOut[32];

        while (true)
        {
            //try {

            //	if (hlp.m_Eh.OptimisedSolve(hlp.m_Blake, fnValid, fnCancelInternal))
            //		break;

            //} catch (const EhSolverCancelledException&) {
            //	return false;
            //}

            //ECC::Hash::Processor hp;
            //Merkle::Hash out;
            //hp << hv << m_Nonce;
            //hp >> out;

            //hlp.Reset(out.m_pData, out.nBytes, m_Nonce);

            // change to use x17r 
            memset(pDataIn, 0x0, 4);
            memcpy(pDataIn + 4, (unsigned char*)pPrev, nSizePrev);
            memset(pDataIn + 36, 0x0, 4);

            memcpy(pDataIn + 40, (unsigned char*)pInput, nSizeInput);
            memcpy(pDataIn + 72, (unsigned char*)m_Nonce.m_pData, m_Nonce.nBytes);

            // progpow fork
            if (height < Rules::get().ProgPowForkHeight) {
                x17r_hash(pDataOut, pDataIn, 80);
            }
            else {
                std::string s = to_hex(pDataIn, 72);

                ECC::Hash::Processor hp;
                Merkle::Hash o;

                hp << s >> o;
                s = to_hex(o.m_pData, o.nBytes);

                uint64_t n =
                    (uint64_t)m_Nonce.m_pData[0] << 56 +
                    (uint64_t)m_Nonce.m_pData[1] << 48 +
                    (uint64_t)m_Nonce.m_pData[2] << 40 +
                    (uint64_t)m_Nonce.m_pData[3] << 32 +
                    (uint64_t)m_Nonce.m_pData[4] << 24 +
                    (uint64_t)m_Nonce.m_pData[5] << 16 +
                    (uint64_t)m_Nonce.m_pData[6] << 8 +
                    (uint64_t)m_Nonce.m_pData[7];
                   
                std::string r;
                progpow_hash(s, n, r);
                bool f;
                auto bytes_vec = from_hex(r, &f);
                assert(bytes_vec.size() == 32);
                for (int i = 0; i < 32; ++i) {
                    pDataOut[i] = bytes_vec[i];
                }
            }

            if (hlp.TestDifficulty(pDataOut, 32, m_Difficulty)) {
                break;
            }

            if (fnCancel(true))
                return false; // retry not allowed

            m_Nonce.Inc();
        }

        return true;
    }

    //bool Block::PoW::IsValid(const void* pInput, uint32_t nSizeInput, Height h) const
    //{
    //	Helper hlp;
    //	hlp.Reset(pInput, nSizeInput, m_Nonce, h);
    //
    //	std::vector<uint8_t> v(m_Indices.begin(), m_Indices.end());
    //    return
    //		hlp.getCurrentPoW(h)->IsValidSolution(hlp.m_Blake, v) &&
    //		hlp.TestDifficulty(&m_Indices.front(), (uint32_t) m_Indices.size(), m_Difficulty);
    //}

    bool Block::PoW::IsValid(const void* pInput, uint32_t nSizeInput, Height h) const
    {
        Helper hlp;
        return hlp.TestDifficulty((const uint8_t*)pInput, nSizeInput, m_Difficulty);
    }
} // namespace ufo

