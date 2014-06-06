/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/containers.h>

namespace et
{
	class HTTPRequestResponse;
	typedef IntrusivePtr<HTTPRequestResponse> HTTPRequestResponsePointer;
	
	class HTTPRequestPrivate;
	class HTTPRequest : public Shared
	{
	public:
		ET_DECLARE_POINTER(HTTPRequest)
		
		enum
		{
			ResponseSuccessful = 200
		};
		
	public:
		HTTPRequest(const std::string& url);
		~HTTPRequest();
		
		void setCredentials(const std::string&, const std::string&);
		void setParameter(const std::string&, const char*, size_t);
		void setParameter(const std::string&, const Dictionary&);
		
		void perform();
		
		HTTPRequestResponsePointer response();
		
		bool succeeded() const;
		
	private:
		friend int HTTPRequestProgressFunction(void*, double, double, double, double);
		HTTPRequestPrivate* _private = nullptr;
		char _privateData[256] = { };
	};
	
	class HTTPRequestResponse : public Shared
	{
	public:
		ET_DECLARE_POINTER(HTTPRequestResponse)
		
	public:
		const BinaryDataStorage& data() const
			{ return _data; }
		
		long responseCode() const
			{ return _responseCode; }
		
	private:
		friend class HTTPRequest;
		friend size_t HTTPRequestWriteFunction(const void*, size_t, size_t, void*);
		
	private:
		BinaryDataStorage _data;
		long _responseCode = 0;
	};
}