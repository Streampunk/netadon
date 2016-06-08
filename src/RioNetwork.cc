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

#define _WIN32_WINNT 0x0603 // required for inet_pton

#include <nan.h>
#include "RioNetwork.h"
#include "Memory.h"

#include <Mswsock.h>
#include <memory>
#include <functional>
#include <iostream>
#include <limits>

namespace streampunk {

static const uint32_t addrPktSize = sizeof(SOCKADDR_INET);
static uint32_t slowStart = 2;

struct EXTENDED_RIO_BUF : public RIO_BUF
{
	OP_TYPE OpType;
  Nan::Callback *SendCallback;
};

std::function<uint32_t(uint32_t,uint32_t)> gcd = [&](uint32_t m, uint32_t n) {
  if (m<n) 
    return gcd(n,m);
  uint32_t remainder(m%n);
  if (0 == remainder)
    return n;
  return gcd(n,remainder);
};

class RioException : public std::exception {
public:
  RioException(std::string msg, int err) {
    char *lpMsgBuf;
    FormatMessage( 
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, err, 
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
      (LPTSTR)&lpMsgBuf,
      0, NULL);
    mMsg = msg + " failed - (" + std::to_string(err) + ") " + std::string(lpMsgBuf);     
    LocalFree(lpMsgBuf);
  }  
  const char *what() const throw()  { return mMsg.c_str(); }

private:
  std::string mMsg;
};


RioNetwork::RioNetwork(std::string ipType, bool reuseAddr, uint32_t packetSize, uint32_t recvMinPackets, uint32_t sendMinPackets)
  : mPacketSize(packetSize), 
    mRecvNumBufs(CalcNumBuffers(packetSize, recvMinPackets)), 
    mSendNumBufs(CalcNumBuffers(packetSize, sendMinPackets)), 
    mAddrNumBufs(CalcNumBuffers(addrPktSize, mSendNumBufs)), 
    mSendIndex(0), mAddrIndex(0),
    mSocket(INVALID_SOCKET), mIOCP(INVALID_HANDLE_VALUE), mCQ(RIO_INVALID_CQ), mRQ(RIO_INVALID_RQ), 
    mRecvBuffID(RIO_INVALID_BUFFERID), mRecvBufs(NULL),
    mSendBuffID(RIO_INVALID_BUFFERID), mSendBufs(NULL),
    mAddrBuffID(RIO_INVALID_BUFFERID), mAddrBufs(NULL) {
  try {
    if (ipType.compare("udp4"))
      throw std::runtime_error("Supports udp4 network only");
    
    InitialiseWinsock();
    InitialiseRIO();

    CreateCompletionQueue();
    CreateRequestQueue();

    InitialiseBuffer(mPacketSize, mRecvNumBufs, mRecvBuff, mRecvBuffID, mRecvBufs, OP_RECV);
    InitialiseBuffer(mPacketSize, mSendNumBufs, mSendBuff, mSendBuffID, mSendBufs, OP_SEND);
    InitialiseBuffer(addrPktSize, mAddrNumBufs, mAddrBuff, mAddrBuffID, mAddrBufs, OP_NONE);

    SetSocketRecvBuffer(mRecvBuff->numBytes());
    SetSocketSendBuffer(mSendBuff->numBytes());
  } catch (RioException& err) {
    throw std::runtime_error(err.what());
  }
}

RioNetwork::~RioNetwork() {
  if (INVALID_SOCKET != mSocket)
    if (SOCKET_ERROR == closesocket(mSocket))
      printf("Error closing socket: %u\n", WSAGetLastError());
  mRio.RIOCloseCompletionQueue(mCQ);
  mRio.RIODeregisterBuffer(mRecvBuffID);
  mRio.RIODeregisterBuffer(mSendBuffID);
  mRio.RIODeregisterBuffer(mAddrBuffID);
  VirtualFree(mRecvBuff->buf(), 0, MEM_RELEASE);
  VirtualFree(mSendBuff->buf(), 0, MEM_RELEASE);
  VirtualFree(mAddrBuff->buf(), 0, MEM_RELEASE);
  delete[] mRecvBufs;  
  delete[] mSendBufs;  
  delete[] mAddrBufs;  
  WSACleanup();
}

void RioNetwork::AddMembership(std::string mAddrStr, std::string uAddrStr) {
  try {
    in_addr maddr;
    inet_pton(AF_INET, mAddrStr.c_str(), (void*)&maddr.s_addr);
    in_addr uaddr;
    inet_pton(AF_INET, uAddrStr.c_str(), (void*)&uaddr.s_addr);

    IP_MREQ mcast;
    mcast.imr_multiaddr = maddr;
    mcast.imr_interface = uaddr;
    if (SOCKET_ERROR == setsockopt(mSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<char *>(&mcast), sizeof(mcast)))
      throw RioException("setsockopt Add Membership", WSAGetLastError());
  } catch (RioException& err) {
    throw std::runtime_error(err.what());
  }
}

void RioNetwork::DropMembership(std::string mAddrStr, std::string uAddrStr) {
  try {
    in_addr maddr;
    inet_pton(AF_INET, mAddrStr.c_str(), (void*)&maddr.s_addr);
    in_addr uaddr;
    inet_pton(AF_INET, uAddrStr.c_str(), (void*)&uaddr.s_addr);

    IP_MREQ mcast;
    mcast.imr_multiaddr = maddr;
    mcast.imr_interface = uaddr;
    if (SOCKET_ERROR == setsockopt(mSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, reinterpret_cast<char *>(&mcast), sizeof(mcast)))
      throw RioException("setsockopt Drop Membership", WSAGetLastError());
  } catch (RioException& err) {
    throw std::runtime_error(err.what());
  }
  
}

void RioNetwork::SetTTL(uint32_t ttl) {
  try {
    if (SOCKET_ERROR == setsockopt(mSocket, IPPROTO_IP, IP_TTL, reinterpret_cast<char *>(&ttl), sizeof(ttl)))
      throw RioException("setsockopt TTL", WSAGetLastError());
  } catch (RioException& err) {
    throw std::runtime_error(err.what());
  }
}

void RioNetwork::SetMulticastTTL(uint32_t ttl) {
  try {
    if (SOCKET_ERROR == setsockopt(mSocket, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast<char *>(&ttl), sizeof(ttl)))
      throw RioException("setsockopt Multicast TTL", WSAGetLastError());
  } catch (RioException& err) {
    throw std::runtime_error(err.what());
  }
}

void RioNetwork::SetBroadcast(bool flag) {
  try {
    if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char *>(&flag), sizeof(flag)))
      throw RioException("setsockopt Broadcast", WSAGetLastError());
  } catch (RioException& err) {
    throw std::runtime_error(err.what());
  }
}

void RioNetwork::SetMulticastLoopback(bool flag) {
  try {
    if (SOCKET_ERROR == setsockopt(mSocket, IPPROTO_IP, IP_MULTICAST_LOOP, reinterpret_cast<char *>(&flag), sizeof(flag)))
      throw RioException("setsockopt Multicast Loop", WSAGetLastError());
  } catch (RioException& err) {
    throw std::runtime_error(err.what());
  }
}

void RioNetwork::Bind(uint32_t &port, std::string &addrStr) {
  try {
    InitialiseRcvs();

    if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&mReuseAddr), sizeof(mReuseAddr)))
      throw RioException("setsockopt reuse address", WSAGetLastError());

    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    if (addrStr.empty())
      addr.sin_addr.s_addr = INADDR_ANY;
    else
      inet_pton(AF_INET, addrStr.c_str(), (void*)&addr.sin_addr);
    addr.sin_port = htons(port);

    if (SOCKET_ERROR == bind(mSocket, reinterpret_cast<SOCKADDR *>(&addr), sizeof(addr)))
      throw RioException("bind", WSAGetLastError());

    int nameLen = sizeof(addr);
    if (getsockname(mSocket, reinterpret_cast<SOCKADDR *>(&addr), &nameLen))
      throw RioException("getsockname", WSAGetLastError());
    port = ntohs(addr.sin_port);

    uint32_t addrSize = 16;
    addrStr.resize(addrSize); 
    inet_ntop(AF_INET, (void*)&addr.sin_addr, (PSTR)addrStr.data(), addrSize);
    addrStr.resize(strlen(addrStr.c_str()));
  } catch (RioException& err) {
    throw std::runtime_error(err.what());
  }
}

void RioNetwork::Send(std::shared_ptr<Memory> data, uint32_t port, std::string addrStr, Nan::Callback *callback) {
  SOCKADDR_IN addr;
  addr.sin_family = AF_INET;
  inet_pton(AF_INET, addrStr.c_str(), (void*)&addr.sin_addr);
  addr.sin_port = htons(port);

  uint32_t aIndex = InterlockedIncrement(&mAddrIndex);
  EXTENDED_RIO_BUF *pAddrBuf = &mAddrBufs[aIndex%mAddrNumBufs];
  memset(mAddrBuff->buf() + pAddrBuf->Offset, 0, addrPktSize);
  if (memcpy_s(mAddrBuff->buf() + pAddrBuf->Offset, addrPktSize, &addr.sin_family, sizeof(SOCKADDR_IN)))
    throw std::runtime_error("memcpy_s failed");
  
  uint32_t dataBytes = data->numBytes();
  uint8_t *srcBuf = data->buf();
  try {
    while (dataBytes) {
      uint32_t index = InterlockedIncrement(&mSendIndex);
      EXTENDED_RIO_BUF *pBuf = &mSendBufs[index%mSendNumBufs];
      uint32_t thisBytes = std::min<uint32_t>(dataBytes, mPacketSize);
      if (memcpy_s(mSendBuff->buf() + pBuf->Offset, mPacketSize, srcBuf, thisBytes))
        throw std::runtime_error("memcpy_s failed");
      pBuf->Length = thisBytes;
      pBuf->SendCallback = callback;
      
      if (!mRio.RIOSendEx(mRQ, pBuf, 1, NULL, pAddrBuf, NULL, NULL, 0, pBuf))
        throw RioException("RIOSendEx", WSAGetLastError());

      dataBytes -= thisBytes;
      srcBuf += thisBytes;
      
      // Appears to need a bit of time to get going or completions arrive with a 'bad parameter' status ??
      if (slowStart) {
        Sleep(10);
        --slowStart;
      }
    }
  } catch (RioException& err) {
    throw std::runtime_error(err.what());
  }
}

void RioNetwork::Close() {
  try {
    if (!::PostQueuedCompletionStatus(mIOCP, 0, 0, 0))
      throw RioException("PostQueuedCompletionStatus", GetLastError());
  } catch (RioException& err) {
    throw std::runtime_error(err.what());
  }
}

bool RioNetwork::processCompletions(std::string &errStr, std::shared_ptr<Memory> &dstBuf, std::vector<Nan::Callback *>&sendCallbacks) {
  const DWORD RIO_MAX_RESULTS = 1000;

  RIORESULT results[RIO_MAX_RESULTS];
  memset(results, 0, sizeof(results));
  ULONG numResults = 0;

  try {
    INT notifyResult = mRio.RIONotify(mCQ);
    if (ERROR_SUCCESS != notifyResult)
      throw RioException("RIONotify", notifyResult);

    DWORD numBytes = 0;
    ULONG_PTR completionKey = 0;
    OVERLAPPED *pOverlapped = 0;
    if (!::GetQueuedCompletionStatus(mIOCP, &numBytes, &completionKey, &pOverlapped, INFINITE))
      throw RioException("GetQueuedCompletionStatus", GetLastError());
  
    if (0 == completionKey)
      return true;

    numResults = mRio.RIODequeueCompletion(mCQ, results, RIO_MAX_RESULTS);
    if (0 == numResults || RIO_CORRUPT_CQ == numResults)
      throw RioException("RIODequeueCompletion", WSAGetLastError());
  } catch (RioException& err) {
    errStr = err.what();
    return false;
  }

  uint32_t totalRecvBytes = 0;
  std::vector<std::pair<uint32_t, EXTENDED_RIO_BUF *> > recvResults;
  for (DWORD i = 0; i < numResults; ++i) {
    EXTENDED_RIO_BUF *pBuf = reinterpret_cast<EXTENDED_RIO_BUF*>(results[i].RequestContext);
    if (0 != results[i].Status) {
      std::string msg = std::string("Rio queued ") + ((OP_RECV == pBuf->OpType)?"Receive":(OP_SEND==pBuf->OpType)?"Send":"?Addr?");
      errStr = RioException(msg.c_str(), results[i].Status).what();
      return false;
    }

    if (results[i].BytesTransferred) {
      if (pBuf && (OP_RECV == pBuf->OpType)) {
        uint32_t numBytes = results[i].BytesTransferred;
        totalRecvBytes += numBytes;
        recvResults.push_back(std::make_pair(numBytes, pBuf));
      }
      else if (pBuf && (OP_SEND == pBuf->OpType)) {
        sendCallbacks.push_back(pBuf->SendCallback);
      }
    }
    else {
      errStr = std::string("No bytes transferred");
      return false;
    }
  }

  try {
    if (totalRecvBytes) {
      uint32_t resultOffset = 0;
      dstBuf = Memory::makeNew(totalRecvBytes);
      for (std::vector<std::pair<uint32_t, EXTENDED_RIO_BUF *> >::const_iterator it = recvResults.begin(); it != recvResults.end(); ++it) {
        EXTENDED_RIO_BUF *pBuf = it->second;
        memcpy_s(dstBuf->buf() + resultOffset, totalRecvBytes - resultOffset, mRecvBuff->buf() + pBuf->Offset, it->first);

        if (!mRio.RIOReceive(mRQ, pBuf, 1, 0, pBuf))
          throw RioException("RIOReceive", WSAGetLastError());
      }
    }
  } catch (RioException& err) {
    errStr = err.what();
    return false;
  }

  return false;
}

void RioNetwork::InitialiseWinsock() {
  WSADATA wsaData;
  int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (NO_ERROR != result)
    throw RioException("WSAStartup", result);

  mSocket = WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_REGISTERED_IO);
  if (INVALID_SOCKET == mSocket)
    throw RioException("WSASocketW", WSAGetLastError());
}

void RioNetwork::InitialiseRIO() {
  GUID functionTableId = WSAID_MULTIPLE_RIO;
  DWORD dwBytes = 0;
  bool ok = true;
  if (0 != WSAIoctl(mSocket, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER, &functionTableId, sizeof(GUID), 
    (void**)&mRio, sizeof(mRio), &dwBytes, 0, 0))
    throw RioException("WSAIoctl", WSAGetLastError());
}

void RioNetwork::CreateCompletionQueue() {
  mIOCP = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);

  RIO_NOTIFICATION_COMPLETION completionType;
  completionType.Type = RIO_IOCP_COMPLETION;
  completionType.Iocp.IocpHandle = mIOCP;
  completionType.Iocp.CompletionKey = (void*)1;
  completionType.Iocp.Overlapped = &mOverlapped;

  mCQ = mRio.RIOCreateCompletionQueue(mRecvNumBufs + mSendNumBufs, &completionType);
  if (RIO_INVALID_CQ == mCQ)
    throw RioException("RIOCreateCompletionQueue", WSAGetLastError());
}

void RioNetwork::CreateRequestQueue() {
  void *pContext = NULL;
  mRQ = mRio.RIOCreateRequestQueue(mSocket, mRecvNumBufs, 1, mSendNumBufs, 1, mCQ, mCQ, pContext);
  if (RIO_INVALID_RQ == mRQ)
    throw RioException("RIOCreateRequestQueue", WSAGetLastError());
}

uint32_t RioNetwork::CalcNumBuffers(uint32_t packetBytes, uint32_t minPackets) {
  SYSTEM_INFO systemInfo;
  GetSystemInfo(&systemInfo);
  DWORD gran = systemInfo.dwAllocationGranularity;
  printf("CalcNumBuffers: granularity %d\n", gran);
  DWORD rnd = (gran * packetBytes) / gcd(gran, packetBytes);

  DWORD bufferBytes = ((packetBytes * minPackets + rnd - 1) / rnd) * rnd;
  printf("CalcNumBuffers: result %d\n", bufferBytes / packetBytes);
  return bufferBytes / packetBytes;
}

void RioNetwork::InitialiseBuffer(uint32_t packetBytes, uint32_t numBufs, std::shared_ptr<Memory> &buff, RIO_BUFFERID &buffID, EXTENDED_RIO_BUF *&bufs, OP_TYPE op) {
  printf ("Initialising %s buffer: %d buffers of %d bytes\n", (OP_RECV==op)?"receive":(OP_SEND==op)?"send":"addr", numBufs, packetBytes);
  DWORD bufferBytes = packetBytes * numBufs;
  PVOID buf = VirtualAlloc(NULL, bufferBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (!buf)
    throw RioException("VirtualAlloc", GetLastError());

  buff = Memory::makeNew(reinterpret_cast<uint8_t *>(buf), bufferBytes);

  buffID = mRio.RIORegisterBuffer(reinterpret_cast<PCHAR>(buff->buf()), buff->numBytes());
  if (RIO_INVALID_BUFFERID == buffID)
    throw RioException("RIORegisterBuffer", WSAGetLastError());
  
  DWORD offset = 0;
  bufs = new EXTENDED_RIO_BUF[numBufs];
  for (uint32_t i = 0; i < numBufs; ++i) {
    EXTENDED_RIO_BUF *pBuf = bufs + i;

    pBuf->BufferId = buffID;
    pBuf->Offset = offset;
    pBuf->Length = packetBytes;
    pBuf->OpType = op;

    offset += packetBytes;
  }
}

void RioNetwork::InitialiseRcvs() {
  DWORD offset = 0;
  for (uint32_t i = 0; i < mRecvNumBufs; ++i) {
    EXTENDED_RIO_BUF *pBuf = mRecvBufs + i;
    if (!mRio.RIOReceive(mRQ, pBuf, 1, 0, pBuf))
      throw RioException("RIOReceive", WSAGetLastError());
  }
}

void RioNetwork::SetSocketRecvBuffer(uint32_t numBytes) {
  if (SOCKET_ERROR == ::setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char *>(&numBytes), sizeof(numBytes)))
    throw RioException("SetSocketRecvBuffer", WSAGetLastError());

  uint32_t getNumBytes = 0;
  int len = sizeof(getNumBytes);
  if (SOCKET_ERROR == ::getsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char *>(&getNumBytes), &len))
    throw RioException("GetSocketRecvBuffer", WSAGetLastError());
  printf("Setting Receive buffer size to %d\n", getNumBytes);
}

void RioNetwork::SetSocketSendBuffer(uint32_t numBytes) {
  if (SOCKET_ERROR == ::setsockopt(mSocket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char *>(&numBytes), sizeof(numBytes)))
    throw RioException("SetSocketSendBuffer", WSAGetLastError());

  uint32_t getNumBytes = 0;
  int len = sizeof(getNumBytes);
  if (SOCKET_ERROR == ::getsockopt(mSocket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char *>(&getNumBytes), &len))
    throw RioException("GetSocketSendBuffer", WSAGetLastError());
  printf("Setting Send buffer size to %d\n", getNumBytes);
}

} // namespace streampunk
