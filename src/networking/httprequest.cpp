/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <libcurl/curl.h>
#include <et-ext/networking/httprequest.h>

using namespace et;

class et::HTTPRequestPrivate
{
public:
	std::string url;
	std::string login;
	std::string password;
	
	HTTPRequestResponsePointer response;
};

size_t et::HTTPRequestWriteFunction(const void* data, size_t chunks, size_t chunkSize, void* pointer)
{
	size_t dataSize = chunks * chunkSize;
	
	reinterpret_cast<HTTPRequestPrivate*>(pointer)->response->_data.appendData(data, dataSize);
	
	return dataSize;
}

int et::HTTPRequestProgressFunction(void* data, double downloadSize, double downloaded, double uploadSize, double uploaded)
{
	return 0;
}

HTTPRequest::HTTPRequest(const std::string& url)
{
	static_assert(sizeof(HTTPRequestPrivate) <= sizeof(_privateData), "Insufficient storage for HTTPRequestPrivate");
	
	_private = new(_privateData) HTTPRequestPrivate;
	_private->url = url;
}

HTTPRequest::~HTTPRequest()
{
	_private->~HTTPRequestPrivate();
	memset(_privateData, 0, sizeof(_privateData));
}

void HTTPRequest::setCredentials(const std::string& login, const std::string& password)
{
	_private->login = login;
	_private->password = password;
}

HTTPRequestResponsePointer HTTPRequest::response()
{
	return _private->response;
}

void HTTPRequest::perform()
{
	CURL* curl = curl_easy_init();
	
	curl_easy_setopt(curl, CURLOPT_URL, _private->url.c_str());
	
	if (!_private->login.empty())
		curl_easy_setopt(curl, CURLOPT_USERNAME, _private->login.c_str());
	
	if (!_private->password.empty())
		curl_easy_setopt(curl, CURLOPT_PASSWORD, _private->password.c_str());
	
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
	curl_easy_setopt(curl, CURLOPT_FILE, _private);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HTTPRequestWriteFunction);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, HTTPRequestProgressFunction);
	curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
	
	if (_private->response.invalid())
		_private->response = HTTPRequestResponsePointer::create();
	
	CURLcode result = curl_easy_perform(curl);
	
	if (result == CURLE_OK)
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &_private->response->_responseCode);
	
	curl_easy_cleanup(curl);
}
