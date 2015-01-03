/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et-ext/networking/httprequest.h>

namespace et
{
	class HTTPAsynchronousRequest : public HTTPRequest
	{
	public:
		ET_DECLARE_POINTER(HTTPAsynchronousRequest)
		
	public:
		HTTPAsynchronousRequest(const std::string& url) :
			HTTPRequest(url) { }
		
		template <typename F>
		void perform(F callback)
		{
			HTTPAsynchronousRequest::Pointer holder(this);
			Invocation([this, callback, holder]() mutable
			{
				perform();
				callback(response());
				holder.reset(nullptr);
			}).invokeInRunLoop(sharedHTTPRequestsThread().runLoop());
		}
		
	private:
		void perform() { HTTPRequest::perform(); }
	};
}
