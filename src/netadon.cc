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
#include "uv.h"

using namespace v8;

uv_handle_t* getTcpHandle(void *handleWrap) {
  volatile char *memory = (volatile char *) handleWrap;
  for (volatile uv_handle_t *tcpHandle = (volatile uv_handle_t *) memory; tcpHandle->type != UV_TCP
      || tcpHandle->data != handleWrap || tcpHandle->loop != uv_default_loop(); tcpHandle = (volatile uv_handle_t *) memory) {
    memory++;
  }
  return (uv_handle_t *) memory;
}

NAN_METHOD(SetSocketRecvBuffer) {
  Local<Object> socket = Local<Object>::Cast(info[0]);
  void *sockHandleWrap = Nan::GetInternalFieldPointer(socket, 0);
  uv_handle_t* sockHandle = getTcpHandle(sockHandleWrap);
  uint32_t numBytes = Nan::To<uint32_t>(info[1]).FromJust();

  int res = uv_recv_buffer_size(sockHandle, (int*)&numBytes);
  if (res < 0)
    printf("SetSocketRecvBuffer error: %s\n", uv_strerror(res));
  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(SetSocketSendBuffer) {
  Local<Object> socket = Local<Object>::Cast(info[0]);
  void *sockHandleWrap = Nan::GetInternalFieldPointer(socket, 0);
  uv_handle_t* sockHandle = getTcpHandle(sockHandleWrap);
  uint32_t numBytes = Nan::To<uint32_t>(info[1]).FromJust();

  int res = uv_send_buffer_size(sockHandle, (int*)&numBytes);
  if (res < 0)
    printf("SetSocketSendBuffer error: %s\n", uv_strerror(res));
  info.GetReturnValue().SetUndefined();
}

NAN_MODULE_INIT(Init) {
  streampunk::UdpPort::Init(target);

  Nan::Set(target, Nan::New<String>("setSocketRecvBuffer").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(SetSocketRecvBuffer)).ToLocalChecked());
  Nan::Set(target, Nan::New<String>("setSocketSendBuffer").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(SetSocketSendBuffer)).ToLocalChecked());
}

NODE_MODULE(netadon, Init)
