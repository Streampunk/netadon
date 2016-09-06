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

#ifndef IPROCESS_H
#define IPROCESS_H

#include <memory>

namespace streampunk {

class Memory;
typedef std::vector<std::shared_ptr<Memory> > tBufVec;

class iProcessData {
public:
  virtual ~iProcessData() {}
};

class iProcess {
public:
  virtual ~iProcess() {}  
  virtual void doProcess (std::shared_ptr<iProcessData> processData, std::string &errStr, 
                          tBufVec &bufVec, uint32_t &port, std::string &addrStr) = 0;
};

} // namespace streampunk

#endif
