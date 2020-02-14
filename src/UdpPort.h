/* Copyright 2017 Streampunk Media Ltd.

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
  void doProcess (std::shared_ptr<iProcessData> processData, std::string &errStr, 
                  tBufVec &bufVec, bool &recvArray, uint32_t &port, std::string &addrStr);

private:
  explicit UdpPort(std::string ipType, bool reuseAddr, bool recvArray, uint32_t packetSize, uint32_t recvMinPackets, uint32_t sendMinPackets,
                   Nan::Callback *portCallback, Nan::Callback *callback);
  ~UdpPort();
  void listenLoop();

  static NAN_METHOD(New) {
    if (info.IsConstructCall()) {
      if (info.Length() != 3)
        return Nan::ThrowError("UdpPort constructor expects 3 arguments");
      if (!info[0]->IsObject())
        return Nan::ThrowError("UdpPort constructor expects an object as the first parameter");
      if (!info[1]->IsFunction())
        return Nan::ThrowError("UdpPort constructor requires a valid callback as the second parameter");
      if (!info[2]->IsFunction())
        return Nan::ThrowError("UdpPort constructor requires a valid callback as the third parameter");

      v8::Local<v8::Object> options = v8::Local<v8::Object>::Cast(info[0]);
      v8::Local<v8::String> typeStr = Nan::New<v8::String>("type").ToLocalChecked();
      if (!Nan::Has(options, typeStr).FromJust())
        return Nan::ThrowError("UdpPort constructor requires type string in first parameter");

      v8::String::Utf8Value ipTypeUtf8(v8::Isolate::GetCurrent(), Nan::To<v8::String>(Nan::Get(options, typeStr).ToLocalChecked()).ToLocalChecked());
      std::string ipType = *ipTypeUtf8;
      bool reuseAddr = false;
      v8::Local<v8::String> reuseStr = Nan::New<v8::String>("reuseAddr").ToLocalChecked();
      if (Nan::Has(options, reuseStr).FromJust()) {
        if (Nan::True() == (Nan::To<v8::Boolean>(Nan::Get(options, reuseStr).ToLocalChecked()).ToLocalChecked()))
          reuseAddr = true;
      }

      bool recvArray = false;
      v8::Local<v8::String> recvArrayStr = Nan::New<v8::String>("receiveArray").ToLocalChecked();
      if (Nan::Has(options, recvArrayStr).FromJust()) {
        if (Nan::True() == (Nan::To<v8::Boolean>(Nan::Get(options, recvArrayStr).ToLocalChecked()).ToLocalChecked()))
          recvArray = true;
      }

      uint32_t packetSize = 1500;
      v8::Local<v8::String> packetSizeStr = Nan::New<v8::String>("packetSize").ToLocalChecked();
      if (Nan::Has(options, packetSizeStr).FromJust())
        packetSize = Nan::To<uint32_t>(Nan::Get(options, packetSizeStr).ToLocalChecked()).FromJust();

      uint32_t recvMinPackets = 16384;
      v8::Local<v8::String> recvMinPacketsStr = Nan::New<v8::String>("recvMinPackets").ToLocalChecked();
      if (Nan::Has(options, recvMinPacketsStr).FromJust())
        recvMinPackets = Nan::To<uint32_t>(Nan::Get(options, recvMinPacketsStr).ToLocalChecked()).FromJust();

      uint32_t sendMinPackets = 16384;
      v8::Local<v8::String> sendMinPacketsStr = Nan::New<v8::String>("sendMinPackets").ToLocalChecked();
      if (Nan::Has(options, sendMinPacketsStr).FromJust())
        sendMinPackets = Nan::To<uint32_t>(Nan::Get(options, sendMinPacketsStr).ToLocalChecked()).FromJust();

      Nan::Callback *portCallback = new Nan::Callback(v8::Local<v8::Function>::Cast(info[1]));
      Nan::Callback *callback = new Nan::Callback(v8::Local<v8::Function>::Cast(info[2]));
      try {
        UdpPort *obj = new UdpPort(ipType, reuseAddr, recvArray, packetSize, recvMinPackets, sendMinPackets, portCallback, callback);
        obj->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
      }
      catch (std::runtime_error e) {
        return Nan::ThrowError(e.what());
      }
    } else {
      const int argc = 3;
      v8::Local<v8::Value> argv[] = { info[0], info[1], info[2] };
      v8::Local<v8::Function> cons = Nan::New(constructor());
      info.GetReturnValue().Set(cons->NewInstance(Nan::GetCurrentContext(), argc, argv).ToLocalChecked());
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

  bool mRecvArray;
  MyWorker *mWorker;
  std::shared_ptr<iNetworkDriver> mNetwork;
  std::thread mListenThread;
};

} // namespace streampunk

#endif
