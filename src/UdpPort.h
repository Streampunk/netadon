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

#ifndef UDPPORT_H
#define UDPPORT_H

#include "iProcess.h"
#include <memory>
#include <thread>

namespace streampunk {

class MyWorker;
class iNetworkDriver;

class UdpPort : public Nan::ObjectWrap, public iProcess {
public:
  static NAN_MODULE_INIT(Init);

  // iProcess
  void doProcess (std::shared_ptr<iProcessData> processData, std::string &errStr, tBufVec &bufVec, uint32_t &port, std::string &addrStr);

private:
  explicit UdpPort(std::string ipType, bool reuseAddr, uint32_t packetSize, uint32_t recvMinPackets, uint32_t sendMinPackets,
                   Nan::Callback *portCallback, Nan::Callback *callback);
  ~UdpPort();
  void listenLoop();

  static NAN_METHOD(New) {
    if (info.IsConstructCall()) {
      if (info.Length() != 6)
        return Nan::ThrowError("UdpPort constructor expects 6 arguments");
      if (!info[0]->IsObject())
        return Nan::ThrowError("UdpPort constructor expects an object as the first parameter");
      if (!info[4]->IsFunction())
        return Nan::ThrowError("UdpPort constructor requires a valid callback as the fifth parameter");
      if (!info[5]->IsFunction())
        return Nan::ThrowError("UdpPort constructor requires a valid callback as the sixth parameter");

      v8::Local<v8::Object> options = v8::Local<v8::Object>::Cast(info[0]);
      v8::Local<v8::String> typeStr = Nan::New<v8::String>("type").ToLocalChecked();
      if (!Nan::Has(options, typeStr).FromJust())
        return Nan::ThrowError("UdpPort constructor requires type string in first parameter");

      v8::String::Utf8Value ipTypeUtf8(Nan::To<v8::String>(Nan::Get(options, typeStr).ToLocalChecked()).ToLocalChecked());
      std::string ipType = *ipTypeUtf8;
      bool reuseAddr = false;
      v8::Local<v8::String> reuseStr = Nan::New<v8::String>("reuseAddr").ToLocalChecked();
      if (Nan::Has(options, reuseStr).FromJust()) {
        if (Nan::True() == (Nan::To<v8::Boolean>(Nan::Get(options, reuseStr).ToLocalChecked()).ToLocalChecked()))
          reuseAddr = true;
      }

      uint32_t packetSize = Nan::To<uint32_t>(info[1]).FromJust();
      uint32_t recvMinPackets = Nan::To<uint32_t>(info[2]).FromJust();
      uint32_t sendMinPackets = Nan::To<uint32_t>(info[3]).FromJust();
      Nan::Callback *portCallback = new Nan::Callback(v8::Local<v8::Function>::Cast(info[4]));
      Nan::Callback *callback = new Nan::Callback(v8::Local<v8::Function>::Cast(info[5]));
      try {
        UdpPort *obj = new UdpPort(ipType, reuseAddr, packetSize, recvMinPackets, sendMinPackets, portCallback, callback);
        obj->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
      }
      catch (std::runtime_error e) {
        return Nan::ThrowError(e.what());
      }
    } else {
      const int argc = 6;
      v8::Local<v8::Value> argv[] = { info[0], info[1], info[2], info[3], info[4], info[5] };
      v8::Local<v8::Function> cons = Nan::New(constructor());
      info.GetReturnValue().Set(cons->NewInstance(argc, argv));
    }
  }

  static inline Nan::Persistent<v8::Function> & constructor() {
    static Nan::Persistent<v8::Function> my_constructor;
    return my_constructor;
  }

  static NAN_METHOD(AddMembership);
  static NAN_METHOD(DropMembership);
  static NAN_METHOD(SetTTL);
  static NAN_METHOD(SetMulticastTTL);
  static NAN_METHOD(SetBroadcast);
  static NAN_METHOD(SetMulticastLoopback);
  static NAN_METHOD(Bind);
  static NAN_METHOD(Send);
  static NAN_METHOD(Close);

  MyWorker *mWorker;
  uint32_t mIsBound;
  std::shared_ptr<iNetworkDriver> mNetwork;
  std::thread mListenThread;
};

} // namespace streampunk

#endif
