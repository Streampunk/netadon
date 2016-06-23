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

#include <nan.h>
#include "UdpPort.h"
#include "MyWorker.h"
#include "Memory.h"
#include "RioNetwork.h"
#include "NetworkFactory.h"

using namespace v8;

namespace streampunk {

class UdpPortProcessData : public iProcessData {
public:
  UdpPortProcessData(std::string errStr, std::shared_ptr<Memory> buf) 
    : mErrStr(errStr), mDstBuf(buf) {}
  ~UdpPortProcessData() {}

  std::string errStr() const { return mErrStr; }
  std::shared_ptr<Memory> dstBuf() const { return mDstBuf; }

private:
  std::string mErrStr;
  std::shared_ptr<Memory> mDstBuf;
};

class UdpPortBindProcessData : public iProcessData {
public:
  UdpPortBindProcessData(uint32_t port, std::string addrStr)
    : mPort(port), mAddrStr(addrStr) {}
  ~UdpPortBindProcessData() {}

  uint32_t mPort;
  std::string mAddrStr;
};

class UdpPortSendProcessData : public iProcessData {
public:
  UdpPortSendProcessData(const std::shared_ptr<Memory> buf, uint32_t port, const std::string &addrStr, bool defer) 
    : mBuf(buf), mPort(port), mAddrStr(addrStr), mDefer(false) {}
  ~UdpPortSendProcessData() {}

  const std::shared_ptr<Memory> mBuf;
  const uint32_t mPort;
  const std::string mAddrStr;
  const bool mDefer;
};

class UdpPortCloseProcessData : public iProcessData {
public:
  UdpPortCloseProcessData() {}
  ~UdpPortCloseProcessData() {}
};

UdpPort::UdpPort(std::string ipType, bool reuseAddr, uint32_t packetSize, uint32_t recvMinPackets, uint32_t sendMinPackets, 
                 Nan::Callback *portCallback, Nan::Callback *callback) 
  : mWorker(new MyWorker(callback, portCallback)), mIsBound(false),
    mNetwork(NetworkFactory::createNetwork(ipType, reuseAddr, packetSize, recvMinPackets, sendMinPackets)),
    mListenThread(std::thread(&UdpPort::listenLoop, this)) {
  AsyncQueueWorker(mWorker);
}
UdpPort::~UdpPort() {}

void UdpPort::listenLoop() {
  bool active = true;
  std::string errStr;
  std::shared_ptr<Memory> dstBuf;

  while (active) {
    active = !mNetwork->processCompletions(errStr, dstBuf);
    if (active) {
      if (!errStr.empty() || dstBuf) {
        mWorker->doProcess(std::make_shared<UdpPortProcessData>(errStr, dstBuf), this, NULL);
      }
    }
    else
      mWorker->quit();
  }
}

// iProcess
void UdpPort::doProcess (std::shared_ptr<iProcessData> processData, std::string &errStr, std::shared_ptr<Memory> &dstBuf, uint32_t &port, std::string &addrStr) {
  try {
    std::shared_ptr<UdpPortProcessData> upd = std::dynamic_pointer_cast<UdpPortProcessData>(processData);
    if (upd) {
      errStr = upd->errStr();
      dstBuf = upd->dstBuf();
    }

    std::shared_ptr<UdpPortBindProcessData> ubpd = std::dynamic_pointer_cast<UdpPortBindProcessData>(processData);
    if (ubpd) {
      mNetwork->Bind(ubpd->mPort, ubpd->mAddrStr);
      port = ubpd->mPort;
      addrStr = ubpd->mAddrStr;
      mIsBound = true;
    }

    std::shared_ptr<UdpPortSendProcessData> uspd = std::dynamic_pointer_cast<UdpPortSendProcessData>(processData);
    if (uspd) {
      if (!mIsBound) {
        uint32_t port = 0;
        std::string addr;
        mNetwork->Bind(port, addr);
        printf ("Bind to %s:%d\n", addr.c_str(), port);
        mIsBound = true;
      }

      tBufVec bufVec;
      bufVec.push_back(uspd->mBuf);

      mNetwork->Send(bufVec, uspd->mPort, uspd->mAddrStr);
      if (!uspd->mDefer)
        mNetwork->CommitSend();
    }

    std::shared_ptr<UdpPortCloseProcessData> ucpd = std::dynamic_pointer_cast<UdpPortCloseProcessData>(processData);
    if (ucpd) {
      mNetwork->Close();
    }
  } catch (std::runtime_error& err) {
    errStr = err.what();
  }
}

NAN_METHOD(UdpPort::AddMembership) {
  if (info.Length() != 2)
    return Nan::ThrowError("UdpPort AddMembership expects 2 arguments");
  String::Utf8Value mAddrStr(Nan::To<String>(info[0]).ToLocalChecked());
  String::Utf8Value uAddrStr(Nan::To<String>(info[1]).ToLocalChecked());

  UdpPort *obj = Nan::ObjectWrap::Unwrap<UdpPort>(info.Holder());
  try {
    obj->mNetwork->AddMembership(*mAddrStr, *uAddrStr);
  } catch (std::runtime_error& err) {
    return Nan::ThrowError(Nan::New(err.what()).ToLocalChecked());
  }
  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(UdpPort::DropMembership) {
  if (info.Length() != 2)
    return Nan::ThrowError("UdpPort DropMembership expects 2 arguments");
  String::Utf8Value mAddrStr(Nan::To<String>(info[0]).ToLocalChecked());
  String::Utf8Value uAddrStr(Nan::To<String>(info[1]).ToLocalChecked());

  UdpPort *obj = Nan::ObjectWrap::Unwrap<UdpPort>(info.Holder());
  try {
    obj->mNetwork->DropMembership(*mAddrStr, *uAddrStr);
  } catch (std::runtime_error& err) {
    return Nan::ThrowError(Nan::New(err.what()).ToLocalChecked());
  }
  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(UdpPort::SetTTL) {
  if (info.Length() != 1)
    return Nan::ThrowError("UdpPort SetTTL expects 1 argument");
  uint32_t ttl = Nan::To<uint32_t>(info[0]).FromJust();

  UdpPort *obj = Nan::ObjectWrap::Unwrap<UdpPort>(info.Holder());
  try {
    obj->mNetwork->SetTTL(ttl);
  } catch (std::runtime_error& err) {
    return Nan::ThrowError(Nan::New(err.what()).ToLocalChecked());
  }
  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(UdpPort::SetMulticastTTL) {
  if (info.Length() != 1)
    return Nan::ThrowError("UdpPort SetMulticastTTL expects 1 argument");
  uint32_t ttl = Nan::To<uint32_t>(info[0]).FromJust();

  UdpPort *obj = Nan::ObjectWrap::Unwrap<UdpPort>(info.Holder());
  try {
    obj->mNetwork->SetMulticastTTL(ttl);
  } catch (std::runtime_error& err) {
    return Nan::ThrowError(Nan::New(err.what()).ToLocalChecked());
  }
  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(UdpPort::SetBroadcast) {
  if (info.Length() != 1)
    return Nan::ThrowError("UdpPort SetBroadcast expects 1 argument");
  bool flag = Nan::To<bool>(info[0]).FromJust();

  UdpPort *obj = Nan::ObjectWrap::Unwrap<UdpPort>(info.Holder());
  try {
    obj->mNetwork->SetBroadcast(flag);
  } catch (std::runtime_error& err) {
    return Nan::ThrowError(Nan::New(err.what()).ToLocalChecked());
  }
  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(UdpPort::SetMulticastLoopback) {
  if (info.Length() != 1)
    return Nan::ThrowError("UdpPort SetMulticastLoopback expects 1 argument");
  bool flag = Nan::To<bool>(info[0]).FromJust();

  UdpPort *obj = Nan::ObjectWrap::Unwrap<UdpPort>(info.Holder());
  try {
    obj->mNetwork->SetMulticastLoopback(flag);
  } catch (std::runtime_error& err) {
    return Nan::ThrowError(Nan::New(err.what()).ToLocalChecked());
  }
  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(UdpPort::Bind) {
  if (info.Length() != 3)
    return Nan::ThrowError("UdpPort Bind expects 4 arguments");
  if (!info[2]->IsFunction())
    return Nan::ThrowError("UdpPort Bind requires a valid callback as the third parameter");

  uint32_t port = Nan::To<uint32_t>(info[0]).FromJust();
  String::Utf8Value addrStr(Nan::To<String>(info[1]).ToLocalChecked());
  Local<Function> callback = Local<Function>::Cast(info[2]);

  UdpPort *obj = Nan::ObjectWrap::Unwrap<UdpPort>(info.Holder());
  try {
    obj->mWorker->doProcess(std::make_shared<UdpPortBindProcessData>(port, *addrStr), obj, new Nan::Callback(callback));
  } catch (std::runtime_error& err) {
    return Nan::ThrowError(Nan::New(err.what()).ToLocalChecked());
  }

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(UdpPort::Send) {
  if (info.Length() != 7)
    return Nan::ThrowError("UdpPort Send expects 6 arguments");
  if (!info[0]->IsObject())
    return Nan::ThrowError("UdpPort Send requires a valid buffer as the first parameter");
  if (!info[5]->IsObject())
    return Nan::ThrowError("UdpPort Send requires an object as the fifth parameter");
  if (!info[6]->IsFunction())
    return Nan::ThrowError("UdpPort Send requires a valid callback as the sixth parameter");

  Local<Object> buf = Local<Object>::Cast(info[0]);
  uint32_t offset = Nan::To<uint32_t>(info[1]).FromJust();
  uint32_t length = Nan::To<uint32_t>(info[2]).FromJust();
  uint32_t port = Nan::To<uint32_t>(info[3]).FromJust();
  String::Utf8Value addrStr(Nan::To<String>(info[4]).ToLocalChecked());

  Local<Object> options = Local<Object>::Cast(info[5]);
  Local<String> modeStr = Nan::New<String>("mode").ToLocalChecked();
  if (!Nan::Has(options, modeStr).FromJust())
    return Nan::ThrowError("UdpPort Send requires mode string in the fifth parameter");

  String::Utf8Value modeUtf8(Nan::To<String>(Nan::Get(options, modeStr).ToLocalChecked()).ToLocalChecked());
  std::string mode = *modeUtf8;
  bool defer = (0 == mode.compare("defer"));

  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[6]));

  uint32_t buffLen = (uint32_t)node::Buffer::Length(buf);
  if (offset + length > buffLen)
    return Nan::ThrowError("UdpPort Send - out of range offset/length");
  uint8_t *sendBuf = (uint8_t *)node::Buffer::Data(buf) + offset;

  UdpPort *obj = Nan::ObjectWrap::Unwrap<UdpPort>(info.Holder());
  try {
    obj->mWorker->doProcess(std::make_shared<UdpPortSendProcessData>(Memory::makeNew(sendBuf, length), port, *addrStr, defer), obj, callback);
  } catch (std::runtime_error& err) {
    return Nan::ThrowError(Nan::New(err.what()).ToLocalChecked());
  }

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(UdpPort::Close) {
  UdpPort *obj = Nan::ObjectWrap::Unwrap<UdpPort>(info.Holder());
  try {
    obj->mWorker->doProcess(std::make_shared<UdpPortCloseProcessData>(), obj, NULL);
  } catch (std::runtime_error& err) {
    return Nan::ThrowError(Nan::New(err.what()).ToLocalChecked());
  }
  info.GetReturnValue().SetUndefined();
}

NAN_MODULE_INIT(UdpPort::Init) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("UdpPort").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  SetPrototypeMethod(tpl, "addMembership", AddMembership);
  SetPrototypeMethod(tpl, "dropMembership", DropMembership);
  SetPrototypeMethod(tpl, "setTTL", SetTTL);
  SetPrototypeMethod(tpl, "setMulticastTTL", SetMulticastTTL);
  SetPrototypeMethod(tpl, "setBroadcast", SetBroadcast);
  SetPrototypeMethod(tpl, "setMulticastLoopback", SetMulticastLoopback);
  SetPrototypeMethod(tpl, "bind", Bind);
  SetPrototypeMethod(tpl, "send", Send);
  SetPrototypeMethod(tpl, "close", Close);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("UdpPort").ToLocalChecked(),
    Nan::GetFunction(tpl).ToLocalChecked());
}

} // namespace streampunk
