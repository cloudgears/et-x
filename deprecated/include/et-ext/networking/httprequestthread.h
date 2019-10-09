/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/app/runloop.h>
#include <et/tasks/tasks.h>
#include <et/threading/thread.h>

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
  ThreadResult main();

 private:
  HTTPRequestsRunLoop _runLoop;
};

HTTPRequestsThread& sharedHTTPRequestsThread();

bool shouldTerminateHTTPRequests();
void terminateHTTPRequests();
}  // namespace et