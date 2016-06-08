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

#ifndef RIONETWORK_H
#define RIONETWORK_H

#include <memory>
#include "iNetworkDriver.h"

namespace streampunk {

class Memory;
struct EXTENDED_RIO_BUF;
enum OP_TYPE { OP_NONE = 0,	OP_RECV = 1, OP_SEND	= 2 };

class RioNetwork : public iNetworkDriver {
public:
  RioNetwork(std::string ipType, bool reuseAddr, uint32_t packetSize, uint32_t recvMinPackets, uint32_t sendMinPackets);
  ~RioNetwork();

  void AddMembership(std::string mAddrStr, std::string uAddrStr);
  void DropMembership(std::string mAddrStr, std::string uAddrStr);
  void SetTTL(uint32_t ttl);
  void SetMulticastTTL(uint32_t ttl);
  void SetBroadcast(bool flag);
  void SetMulticastLoopback(bool flag);
  void Bind(uint32_t &port, std::string &addrStr);
  void Send(std::shared_ptr<Memory> data, uint32_t port, std::string addrStr, Nan::Callback *callback);
  void Close();

protected:
  bool processCompletions(std::string &errStr, std::shared_ptr<Memory> &dstBuf, std::vector<Nan::Callback *>&sendCallbacks);
  
private:
  bool mReuseAddr;
  uint32_t mPacketSize;
  uint32_t mRecvNumBufs;
  uint32_t mSendNumBufs;
  uint32_t mAddrNumBufs;
  uint32_t mSendIndex;
  uint32_t mAddrIndex;
  SOCKET mSocket;
  HANDLE mIOCP;
  RIO_EXTENSION_FUNCTION_TABLE mRio;
  RIO_CQ mCQ;
  RIO_RQ mRQ;
  std::shared_ptr<Memory> mRecvBuff;
  std::shared_ptr<Memory> mSendBuff;
  std::shared_ptr<Memory> mAddrBuff;
  RIO_BUFFERID mRecvBuffID;
  RIO_BUFFERID mSendBuffID;
  RIO_BUFFERID mAddrBuffID;
  EXTENDED_RIO_BUF *mRecvBufs;
  EXTENDED_RIO_BUF *mSendBufs;
  EXTENDED_RIO_BUF *mAddrBufs;
  OVERLAPPED mOverlapped;
  
  void InitialiseWinsock();
  void InitialiseRIO();

  void CreateCompletionQueue();
  void CreateRequestQueue();
  uint32_t CalcNumBuffers(uint32_t packetBytes, uint32_t minBufferBytes);
  void InitialiseBuffer(uint32_t packetBytes, uint32_t numBufs, std::shared_ptr<Memory> &buff, RIO_BUFFERID &buffID, EXTENDED_RIO_BUF *&bufs, OP_TYPE op);
  void InitialiseRcvs();
  void SetSocketRecvBuffer(uint32_t numBytes);
  void SetSocketSendBuffer(uint32_t numBytes);
};

} // namespace streampunk

#endif