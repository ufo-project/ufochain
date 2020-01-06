// Copyright 2019 The UFO Team
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

#include "default_peers.h"

namespace ufo
{
    std::vector<std::string> getDefaultPeers()
    {
        std::vector<std::string> result
        {
#ifdef UFO_TESTNET
            "202.181.144.43:20015",
            "202.181.144.44:20015",
            "202.181.144.45:20015"
#else
            
#endif
        };

        return result;
    }

    std::vector<std::string> getOutdatedDefaultPeers()
    {
        std::vector<std::string> result
        {
#if defined(UFO_TESTNET)
            
#elif defined(UFO_MAINNET)
           
#else
            
#endif
        };

        return result;
    }
}
