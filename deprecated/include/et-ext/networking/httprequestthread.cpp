/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/tools.hpp>
#include <et/networking/httprequestthread.hpp>

namespace et {

static bool _shouldTerminateRequests = false;

HTTPRequestsThread& sharedHTTPRequestsThread() {
  static HTTPRequestsThread _thread;
  return _thread;
}

bool shouldTerminateHTTPRequests() {
  return _shouldTerminateRequests;
}

void terminateHTTPRequests() {
  _shouldTerminateRequests = true;
}

HTTPRequestsRunLoop::HTTPRequestsRunLoop()
  : _owner(nullptr) {}

void HTTPRequestsRunLoop::setOwner(HTTPRequestsThread* owner) {
  _owner = owner;
}

void HTTPRequestsRunLoop::addTask(Task* t, float delay) {
  updateTime(queryContinuousTimeInMilliSeconds());
  RunLoop::addTask(t, delay);
  _owner->resume();
}

HTTPRequestsThread::HTTPRequestsThread()
  : Thread() {
  _runLoop.setOwner(this);
}

HTTPRequestsThread::~HTTPRequestsThread() {
  terminateHTTPRequests();
  stop();
  join();
}

void HTTPRequestsThread::main() {
  while (running()) {
    if (_runLoop.hasTasks() || _runLoop.mainTimerPool()->hasObjects()) {
      _runLoop.update(queryContinuousTimeInMilliSeconds());
      std::this_thread::sleep_for(std::chrono::milliseconds(25));
    } else {
      suspend();
    }
  }
}

}  // namespace et
