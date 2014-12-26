/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/tools.h>
#include <et-ext/networking/httprequestthread.h>

using namespace et;

HTTPRequestsThread& et::sharedHTTPRequestsThread()
{
	static HTTPRequestsThread _thread;

	return _thread;
}

HTTPRequestsRunLoop::HTTPRequestsRunLoop() :
	_owner(nullptr) { }

void HTTPRequestsRunLoop::setOwner(HTTPRequestsThread* owner)
	{ _owner = owner; }

void HTTPRequestsRunLoop::addTask(Task* t, float delay)
{
	updateTime(queryContiniousTimeInMilliSeconds());
	RunLoop::addTask(t, delay);
	_owner->resume();
}

HTTPRequestsThread::HTTPRequestsThread() :
	Thread(true)
{
	_runLoop.setOwner(this);
}

ThreadResult HTTPRequestsThread::main()
{
	while (running())
	{
		if (_runLoop.hasTasks() || _runLoop.firstTimerPool()->hasObjects())
		{
			_runLoop.update(queryContiniousTimeInMilliSeconds());
			Thread::sleepMSec(25);
		}
		else
		{
			suspend();
		}
	}
	
	return 0;
}
