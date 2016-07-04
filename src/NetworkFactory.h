/* Copyright 2016 Streampunk Media Ltd.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef NETWORKFACTORY_H
#define NETWORKFACTORY_H

#include <memory>

#if defined _WIN32
  #include "RioNetwork.h"
#elif defined _LINUX
#endif

namespace streampunk {

class iNetworkDriver;

class NetworkFactory {
public:
  static std::shared_ptr<iNetworkDriver> createNetwork(std::string ipType, bool reuseAddr, uint32_t packetSize, uint32_t recvMinPackets, uint32_t sendMinPackets) {
    #if defined _WIN32
      return std::make_shared<RioNetwork>(ipType, reuseAddr, packetSize, recvMinPackets, sendMinPackets);
    #elif defined _LINUX
      throw std::runtime_error("No Linux implementation of iNetworkDriver available");
    #endif

    return std::shared_ptr<iNetworkDriver>();
  }
};

} // namespace streampunk

#endif
