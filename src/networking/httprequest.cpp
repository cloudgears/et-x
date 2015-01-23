/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <external/libcurl/curl.h>

#include <et/json/json.h>
#include <et-ext/networking/httprequest.h>

using namespace et;

static AtomicBool shouldInitCurl = true;

class et::HTTPRequestPrivate
{
public:
	std::string url;
	std::string login;
	std::string password;
	std::map<std::string, BinaryDataStorage> params;
	std::map<std::string, std::string> uploadFiles;
	BinaryDataStorage body;
	
	HTTPRequestResponse::Pointer response;
	
	bool _succeeded = false;
	
	HTTPRequestPrivate(const std::string& aUrl) :
		url(aUrl)
	{
		response = HTTPRequestResponse::Pointer::create();
	}
};

size_t et::HTTPRequestWriteFunction(const void* data, size_t chunks, size_t chunkSize, HTTPRequestPrivate* req)
{
	size_t dataSize = chunks * chunkSize;
	req->response->_data.appendData(data, dataSize);
	return shouldTerminateHTTPRequests() ? 0 : dataSize;
}

int et::HTTPRequestProgressFunction(HTTPRequest* req, double downloadSize, double downloaded, double uploadSize, double uploaded)
{
	if ((downloaded > 0.0) || (downloadSize > 0.0))
		req->downloadProgress.invokeInMainRunLoop(static_cast<int64_t>(downloaded), static_cast<int64_t>(downloadSize));
	
	if ((uploaded > 0.0) || (uploadSize > 0.0))
		req->uploadProgress.invokeInMainRunLoop(static_cast<int64_t>(uploaded), static_cast<int64_t>(uploadSize));
	
	return 0;
}

HTTPRequest::HTTPRequest(const std::string& url)
{
	ET_PIMPL_INIT(HTTPRequest, url)
	
	if (shouldInitCurl)
	{
		curl_global_init(CURL_GLOBAL_ALL);
		shouldInitCurl = false;
	}
}

HTTPRequest::~HTTPRequest()
{
	ET_PIMPL_FINALIZE(HTTPRequest)
}

void HTTPRequest::setCredentials(const std::string& login, const std::string& password)
{
	_private->login = login;
	_private->password = password;
}

void HTTPRequest::addParameter(const std::string& name, const Dictionary& d)
{
	if (name.empty()) return;
	
	addParameter(name, json::serialize(d));
}

void HTTPRequest::addParameter(const std::string& name, const std::string& str)
{
	if (str.empty() || name.empty()) return;
		
	addParameter(name, str.c_str(), str.size());
}

void HTTPRequest::addParameter(const std::string& name, const char* data, size_t dataSize)
{
	if (name.empty() || (data == nullptr) || (dataSize == 0)) return;
	
	_private->params[name].resize(dataSize);
	etCopyMemory(_private->params[name].data(), data, dataSize);
}

void HTTPRequest::addFileParameter(const std::string& name, const std::string& filename)
{
	if (name.empty()) return;
	
	_private->uploadFiles.insert(std::make_pair(name, filename));
}

HTTPRequestResponsePointer HTTPRequest::response()
{
	ET_ASSERT(_private);
	
	return _private->response;
}

void HTTPRequest::perform()
{
	ET_ASSERT(_private);
	
	CURL* curl = curl_easy_init();
	
	curl_easy_setopt(curl, CURLOPT_URL, _private->url.c_str());
	
	if (!_private->login.empty())
		curl_easy_setopt(curl, CURLOPT_USERNAME, _private->login.c_str());
	
	if (!_private->password.empty())
		curl_easy_setopt(curl, CURLOPT_PASSWORD, _private->password.c_str());
	
	if (_private->params.size() + _private->uploadFiles.size() > 0)
	{
		curl_httppost* params = nullptr;
		curl_httppost* lastptr = nullptr;
		
		for (const auto& kv : _private->params)
		{
			curl_formadd(&params, &lastptr, CURLFORM_COPYNAME, kv.first.c_str(),
				CURLFORM_PTRCONTENTS, kv.second.data(), CURLFORM_CONTENTSLENGTH, kv.second.size(), CURLFORM_END);
		}
		
		for (const auto& kv : _private->uploadFiles)
		{
			curl_formadd(&params, &lastptr, CURLFORM_COPYNAME, kv.first.c_str(),
				CURLFORM_FILE, kv.second.c_str(), CURLFORM_END);
		}
		
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, params);
	}
	
	if (_private->body.size() > 0)
	{
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, _private->body.data());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, curl_off_t(_private->body.size()));
	}
	
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10000);
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
	
	_private->_succeeded = (result == CURLE_OK);
	
	unsigned char zero[] = { 0 };
	_private->response->_data.append(zero, sizeof(zero));
	_private->response->_responseCode = 0;
	
	if (_private->_succeeded)
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &_private->response->_responseCode);
	
	curl_easy_cleanup(curl);
}

void HTTPRequest::setBody(const BinaryDataStorage& data)
{
	_private->body.resize(data.size());
	etCopyMemory(_private->body.binary(), data.binary(), data.size());
}

bool HTTPRequest::succeeded() const
{
	return _private->_succeeded;
}
