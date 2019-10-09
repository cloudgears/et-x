/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/app/runloop.hpp>
#include <et/core/tasks.hpp>
#include <et/core/thread.hpp>

namespace et {

class HTTPRequestsThread;

class HTTPRequestsRunLoop : public RunLoop {
 private:
  HTTPRequestsRunLoop();

  void setOwner(HTTPRequestsThread* owner);
  void addTask(Task* t, float);

 private:
  friend class HTTPRequestsThread;
  HTTPRequestsThread* _owner;
};

class HTTPRequestsThread : public Thread {
 public:
  HTTPRequestsThread();
  ~HTTPRequestsThread();

  RunLoop& runLoop() {
    return _runLoop;
  }

 private:
  void main() override;

 private:
  HTTPRequestsRunLoop _runLoop;
};

HTTPRequestsThread& sharedHTTPRequestsThread();

bool shouldTerminateHTTPRequests();
void terminateHTTPRequests();
}  // namespace et