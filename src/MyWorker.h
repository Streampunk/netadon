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

#ifndef MYWORKER_H
#define MYWORKER_H

#include <nan.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <map>

#include "Memory.h"

using namespace v8;

namespace streampunk {

static std::map<char*, std::shared_ptr<Memory> > outstandingAllocs;
static void freeAllocCb(char* data, void* hint) {
  std::map<char*, std::shared_ptr<Memory> >::iterator it = outstandingAllocs.find(data);
  outstandingAllocs.erase(it);
}

template <class T>
class WorkQueue {
public:
  WorkQueue() : qu(), m(), cv() {}
  ~WorkQueue() {}
  
  void enqueue(T t) {
    std::lock_guard<std::mutex> lk(m);
    qu.push(t);
    cv.notify_one();
  }
  
  T dequeue() {
    std::unique_lock<std::mutex> lk(m);
    while(qu.empty()) {
      cv.wait(lk);
    }
    T val = qu.front();
    qu.pop();
    return val;
  }

  size_t size() {
    std::lock_guard<std::mutex> lk(m);
    return qu.size();
  }

private:
  std::queue<T> qu;
  std::mutex m;
  std::condition_variable cv;
};

class iProcess;
class iProcessData;
class MyWorker : public Nan::AsyncProgressWorker {
public:
  MyWorker(Nan::Callback *callback)
    : Nan::AsyncProgressWorker(callback), mActive(true), mProgressCallback(NULL) {}
  ~MyWorker() {
    delete mProgressCallback;
  }

  void setProgressCallback(Nan::Callback *callback) {
    mProgressCallback = callback;
  }

  uint32_t numQueued() {
    return (uint32_t)mWorkQueue.size();
  }

  void doProcess(std::shared_ptr<iProcessData> processData, iProcess *process) {
    mWorkQueue.enqueue (
      std::make_shared<WorkParams>(processData, process));
  }

  void quit() {
    mWorkQueue.enqueue (std::make_shared<WorkParams>(std::shared_ptr<iProcessData>(), (iProcess *)NULL));
  }

private:  
  void Execute(const ExecutionProgress& progress) {
    // Asynchronous, non-V8 work goes here
    while (mActive) {
      std::shared_ptr<WorkParams> wp = mWorkQueue.dequeue();
      if (wp->mProcess)
        wp->mProcess->doProcess(wp->mProcessData, wp->mErrStr, wp->mPort, wp->mAddrStr);
      else
        mActive = false;
      mDoneQueue.enqueue(wp);
      progress.Send(NULL, 0);
    }

    // wait for quit message to be passed to callback
    std::unique_lock<std::mutex> lk(mMtx);
    mCv.wait(lk);
  }
  
  void HandleProgressCallback(const char *data, size_t size) {
    Nan::HandleScope scope;
    while (mDoneQueue.size() != 0)
    {
      std::shared_ptr<WorkParams> wp = mDoneQueue.dequeue();
      if (!wp->mErrStr.empty() && mProgressCallback) {
        Local<Value> argv[] = { Nan::New(wp->mErrStr.c_str()).ToLocalChecked() };
        mProgressCallback->Call(1, argv);
      }
      else if (wp->mProcessData && wp->mProcessData->dstBuf()) {
        std::shared_ptr<Memory> resultMem = wp->mProcessData->dstBuf();
        outstandingAllocs.insert(make_pair((char*)resultMem->buf(), resultMem));
        Nan::MaybeLocal<v8::Object> maybeBuf = Nan::NewBuffer((char*)resultMem->buf(), resultMem->numBytes(), freeAllocCb, 0);
        if (mProgressCallback) {
          Local<Value> argv[] = { Nan::Null(), maybeBuf.ToLocalChecked() };
          mProgressCallback->Call(2, argv);
        }
      }
      else if (wp->mProcessData && wp->mProcessData->sendCallback()) {
        Local<Value> argv[] = { Nan::Null() };
        wp->mProcessData->sendCallback()->Call(1, argv);
      }
      else if (!wp->mAddrStr.empty() && mProgressCallback) {
        Local<Value> argv[] = { Nan::Null(), Nan::Null(), Nan::New(wp->mPort), Nan::New(wp->mAddrStr).ToLocalChecked() };
        mProgressCallback->Call(4, argv);
      } 

      if (!wp->mProcess && !mActive) {
        if (mProgressCallback) {
          Local<Value> argv[] = { Nan::Null(), Nan::Null() };
          mProgressCallback->Call(2, argv);
        }

        // notify the thread to exit
        std::unique_lock<std::mutex> lk(mMtx);
        mCv.notify_one();
      }
    }
  }
  
  void HandleOKCallback() {
    Nan::HandleScope scope;
    callback->Call(0, NULL);
  }

  bool mActive;
  Nan::Callback *mProgressCallback;
  struct WorkParams {
    WorkParams(std::shared_ptr<iProcessData> processData, iProcess *process)
      : mProcessData(processData), mProcess(process) {}
    ~WorkParams() {}

    std::shared_ptr<iProcessData> mProcessData;
    iProcess *mProcess;
    std::string mErrStr;
    uint32_t mPort;
    std::string mAddrStr;
  };
  WorkQueue<std::shared_ptr<WorkParams> > mWorkQueue;
  WorkQueue<std::shared_ptr<WorkParams> > mDoneQueue;
  std::mutex mMtx;
  std::condition_variable mCv;
};

} // namespace streampunk

#endif
