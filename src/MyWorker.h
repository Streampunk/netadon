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
  if (it != outstandingAllocs.end())
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

  size_t size() const {
    std::lock_guard<std::mutex> lk(m);
    return qu.size();
  }

private:
  std::queue<T> qu;
  mutable std::mutex m;
  std::condition_variable cv;
};

class iProcess;
class iProcessData;
class MyWorker : public Nan::AsyncProgressWorker {
public:
  MyWorker(Nan::Callback *callback, Nan::Callback *progressCallback)
    : Nan::AsyncProgressWorker(callback), mActive(true), mProgressCallback(progressCallback) {}
  ~MyWorker() {
    delete mProgressCallback;
  }

  uint32_t numQueued() {
    return (uint32_t)mWorkQueue.size();
  }

  void doProcess(std::shared_ptr<iProcessData> processData, iProcess *process, Nan::Callback *doneCallback) {
    mWorkQueue.enqueue (
      std::make_shared<WorkParams>(processData, process, doneCallback));
  }

  void quit() {
    mWorkQueue.enqueue (std::make_shared<WorkParams>(std::shared_ptr<iProcessData>(), (iProcess *)NULL, (Nan::Callback *)NULL));
  }

private:  
  void Execute(const ExecutionProgress& progress) {
    // Asynchronous, non-V8 work goes here
    while (mActive) {
      std::shared_ptr<WorkParams> wp = mWorkQueue.dequeue();
      if (wp->mProcess)
        wp->mProcess->doProcess(wp->mProcessData, wp->mErrStr, wp->mBufVec, wp->mPort, wp->mAddrStr);
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
      
      if (!wp->mErrStr.empty()) {
        printf("Error: %s\n", wp->mErrStr.c_str());
        
        Local<Value> argv[] = { Nan::New(wp->mErrStr.c_str()).ToLocalChecked() };
        mProgressCallback->Call(1, argv);
      }
      else if (wp->mCallback) {
        if (!wp->mAddrStr.empty()) {
          Local<Value> argv[] = { Nan::Null(), Nan::New(wp->mPort), Nan::New(wp->mAddrStr).ToLocalChecked() };
          wp->mCallback->Call(3, argv);
        }
        else {
          Local<Value> argv[] = { Nan::Null() };
          wp->mCallback->Call(1, argv);
        }
      }
      else for (tBufVec::const_iterator it = wp->mBufVec.begin(); it != wp->mBufVec.end(); ++it) {
        std::shared_ptr<Memory> resultMem = *it;
        outstandingAllocs.insert(make_pair((char*)resultMem->buf(), resultMem));
        Nan::MaybeLocal<v8::Object> maybeBuf = Nan::NewBuffer((char*)resultMem->buf(), resultMem->numBytes(), freeAllocCb, 0);
        Local<Value> argv[] = { Nan::Null(), maybeBuf.ToLocalChecked() };
        mProgressCallback->Call(2, argv);
      }

      if (!wp->mProcess && !mActive) {
        Local<Value> argv[] = { Nan::Null() };
        mProgressCallback->Call(1, argv);

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
    WorkParams(std::shared_ptr<iProcessData> processData, iProcess *process, Nan::Callback *callback)
      : mProcessData(processData), mProcess(process), mCallback(callback), mPort(0) {}
    ~WorkParams() { delete mCallback; }

    std::shared_ptr<iProcessData> mProcessData;
    iProcess *mProcess;
    Nan::Callback *mCallback;
    std::string mErrStr;
    tBufVec mBufVec; 
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
