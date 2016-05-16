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

namespace streampunk {

class MyWorker;
class RioNetwork;
class iNetworkDriver;

class UdpPort : public Nan::ObjectWrap, public iProcess {
public:
  static NAN_MODULE_INIT(Init);

  // iProcess
  uint32_t doProcess (std::shared_ptr<iProcessData> processData, std::string &errStr, uint32_t &port, std::string &addrStr);
  
private:
  explicit UdpPort(std::string ipType, uint32_t packetSize, uint32_t recvMinPackets, uint32_t sendMinPackets, Nan::Callback *callback);
  ~UdpPort();

  static NAN_METHOD(New) {
    if (info.IsConstructCall()) {
      if (!((info.Length() == 5) && (info[4]->IsFunction())))
        return Nan::ThrowError("UdpPort constructor requires a valid callback as the fifth parameter");

      v8::String::Utf8Value ipType(Nan::To<v8::String>(info[0]).ToLocalChecked());
      uint32_t packetSize = Nan::To<uint32_t>(info[1]).FromJust();
      uint32_t recvMinPackets = Nan::To<uint32_t>(info[2]).FromJust();
      uint32_t sendMinPackets = Nan::To<uint32_t>(info[3]).FromJust();
      Nan::Callback *callback = new Nan::Callback(v8::Local<v8::Function>::Cast(info[4]));
      UdpPort *obj = new UdpPort(*ipType, packetSize, recvMinPackets, sendMinPackets, callback);
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    } else {
      const int argc = 5;
      v8::Local<v8::Value> argv[] = { info[0], info[1], info[2], info[3], info[4] };
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
  std::shared_ptr<iNetworkDriver> mNetwork;
  //RioNetwork *mRioNetwork;
};

} // namespace streampunk

#endif