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
			Invocation([this, callback]()
			{
				perform();
				callback(response());
			}).invokeInRunLoop(sharedHTTPRequestsThread().runLoop());
		}
		
	private:
		void perform() { HTTPRequest::perform(); }
	};
}
