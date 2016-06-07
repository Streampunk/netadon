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
  UdpPortProcessData() {}
  ~UdpPortProcessData() {}

  void setDstBuf(std::shared_ptr<Memory> buf) { mDstBuf = buf; }  
  std::shared_ptr<Memory> dstBuf() const { return mDstBuf; }

private:
  std::shared_ptr<Memory> mDstBuf;
};

class UdpPortBindProcessData : public iProcessData {
public:
  UdpPortBindProcessData(uint32_t port, std::string addrStr)
    : mPort(port), mAddrStr(addrStr) {}
  ~UdpPortBindProcessData() {}

  std::shared_ptr<Memory> dstBuf() const { return std::shared_ptr<Memory>(); }

  uint32_t mPort;
  std::string mAddrStr;
};


UdpPort::UdpPort(std::string ipType, bool reuseAddr, uint32_t packetSize, uint32_t recvMinPackets, uint32_t sendMinPackets, Nan::Callback *callback) 
  : mWorker(new MyWorker(callback)) {
  AsyncQueueWorker(mWorker);
  try {
    mNetwork = NetworkFactory::createNetwork(ipType, reuseAddr, packetSize, recvMinPackets, sendMinPackets);
  } catch (std::runtime_error& err) {
    Nan::ThrowError(Nan::New(err.what()).ToLocalChecked());
  }
}
UdpPort::~UdpPort() {}

// iProcess
uint32_t UdpPort::doProcess (std::shared_ptr<iProcessData> processData, std::string &errStr, uint32_t &port, std::string &addrStr) {
  std::shared_ptr<Memory> dstBuf;
  std::shared_ptr<UdpPortProcessData> upd = std::dynamic_pointer_cast<UdpPortProcessData>(processData);
  if (upd) {
    bool close = mNetwork->processCompletions(errStr, dstBuf);
    upd->setDstBuf(dstBuf);

    if (close)
      mWorker->quit();
    else  
      mWorker->doProcess(std::make_shared<UdpPortProcessData>(), this);
  }

  std::shared_ptr<UdpPortBindProcessData> ubpd = std::dynamic_pointer_cast<UdpPortBindProcessData>(processData);
  if (ubpd) {
    mWorker->doProcess(std::make_shared<UdpPortProcessData>(), this);
    mNetwork->Bind(ubpd->mPort, ubpd->mAddrStr);
    port = ubpd->mPort;
    addrStr = ubpd->mAddrStr;
  }

  return dstBuf?dstBuf->numBytes():0;
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
    return Nan::ThrowError("UdpPort Bind expects 3 arguments");
  if (!info[2]->IsFunction())
    return Nan::ThrowError("UdpPort Bind requires a valid callback as the third parameter");

  uint32_t port = Nan::To<uint32_t>(info[0]).FromJust();
  String::Utf8Value addrStr(Nan::To<String>(info[1]).ToLocalChecked());
  Local<Function> callback = Local<Function>::Cast(info[2]);

  UdpPort *obj = Nan::ObjectWrap::Unwrap<UdpPort>(info.Holder());

  try {
    obj->mWorker->setProgressCallback(new Nan::Callback(callback));
    obj->mWorker->doProcess(std::make_shared<UdpPortBindProcessData>(port, *addrStr), obj);
  } catch (std::runtime_error& err) {
    return Nan::ThrowError(Nan::New(err.what()).ToLocalChecked());
  }

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(UdpPort::Send) {
  if (info.Length() != 5)
    return Nan::ThrowError("UdpPort Send expects 5 arguments");
  if (!info[0]->IsObject())
    return Nan::ThrowError("UdpPort Send requires a valid buffer as the first parameter");

  Local<Object> buf = Local<Object>::Cast(info[0]);
  uint32_t offset = Nan::To<uint32_t>(info[1]).FromJust();
  uint32_t length = Nan::To<uint32_t>(info[2]).FromJust();
  uint32_t port = Nan::To<uint32_t>(info[3]).FromJust();
  String::Utf8Value addrStr(Nan::To<String>(info[4]).ToLocalChecked());

  uint32_t buffLen = (uint32_t)node::Buffer::Length(buf);
  if (offset + length > buffLen)
    return Nan::ThrowError("UdpPort Send - out of range offset/length");
  uint8_t *sendBuf = (uint8_t *)node::Buffer::Data(buf) + offset;

  UdpPort *obj = Nan::ObjectWrap::Unwrap<UdpPort>(info.Holder());
  try {
    obj->mNetwork->Send(Memory::makeNew(sendBuf, length), port, *addrStr);
  } catch (std::runtime_error& err) {
    return Nan::ThrowError(Nan::New(err.what()).ToLocalChecked());
  }

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(UdpPort::Close) {
  UdpPort *obj = Nan::ObjectWrap::Unwrap<UdpPort>(info.Holder());
  try {
    obj->mNetwork->Close();
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
