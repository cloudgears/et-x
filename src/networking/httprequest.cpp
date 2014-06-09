/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <libcurl/curl.h>
#include <et-ext/json/json.h>
#include <et-ext/networking/httprequest.h>

using namespace et;

static int my_trace(CURL *handle, curl_infotype type, char *data, size_t size, void *userp);
static void dump(const char *text, FILE *stream, unsigned char *ptr, size_t size, char nohex);

class et::HTTPRequestPrivate
{
public:
	std::string url;
	std::string login;
	std::string password;
	std::map<std::string, BinaryDataStorage> params;
	std::map<std::string, std::string> uploadFiles;
	
	HTTPRequestResponsePointer response;
	
	bool _succeeded = false;
};

size_t et::HTTPRequestWriteFunction(const void* data, size_t chunks, size_t chunkSize, HTTPRequestPrivate* req)
{
	size_t dataSize = chunks * chunkSize;
	req->response->_data.appendData(data, dataSize);
	return dataSize;
}

int et::HTTPRequestProgressFunction(HTTPRequest* req, double downloadSize, double downloaded, double uploadSize, double uploaded)
{
	if (downloadSize > 0)
		req->downloadProgress.invokeInMainRunLoop(downloaded, downloadSize);
	
	if (uploadSize > 0)
		req->uploadProgress.invokeInMainRunLoop(uploaded, uploadSize);
	
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

void HTTPRequest::addParameter(const std::string& name, const Dictionary& d)
{
	addParameter(name, json::serialize(d));
}

void HTTPRequest::addParameter(const std::string& name, const std::string& str)
{
	addParameter(name, str.c_str(), str.size());
}

void HTTPRequest::addParameter(const std::string& name, const char* data, size_t dataSize)
{
	_private->params[name].resize(dataSize);
	etCopyMemory(_private->params[name].data(), data, dataSize);
}

void HTTPRequest::addFileParameter(const std::string& param, const std::string& filename)
{
	_private->uploadFiles.insert(std::make_pair(param, filename));
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
	
	if (_private->params.size() + _private->uploadFiles.size() > 0)
	{
		curl_httppost* params = nullptr;
		curl_httppost* lastptr = nullptr;
		
		for (const auto& kv : _private->params)
		{
			curl_formadd(&params, &lastptr, CURLFORM_PTRNAME, kv.first.c_str(), CURLFORM_PTRCONTENTS,
				kv.second.data(), CURLFORM_CONTENTSLENGTH, kv.second.size(), CURLFORM_END);
		}
		
		for (const auto& kv : _private->uploadFiles)
		{
			curl_formadd(&params, &lastptr, CURLFORM_PTRNAME, kv.first.c_str(), CURLFORM_FILE,
				kv.second.c_str(), CURLFORM_END);
		}
		
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, params);
	}
		
//	curl_easy_setopt(curl, CURLOPT_POST, 1);
//	curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
//	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
//	curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 750);
	
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
	
	if (_private->_succeeded)
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &_private->response->_responseCode);
	
	curl_easy_cleanup(curl);
}

bool HTTPRequest::succeeded() const
{
	return _private->_succeeded;
}

/*
 * service
 */
struct data {
	char trace_ascii; /* 1 or 0 */
};

static void dump(const char *text, FILE *stream, unsigned char *ptr, size_t size, char nohex)
{
	size_t i;
	size_t c;
	
	unsigned int width=0x10;
	
	if(nohex)
    /* without the hex output, we can fit more on screen */
		width = 0x40;
	
	fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
			text, (long)size, (long)size);
	
	for(i=0; i<size; i+= width) {
		
		fprintf(stream, "%4.4lx: ", (long)i);
		
		if(!nohex) {
			/* hex not disabled, show it */
			for(c = 0; c < width; c++)
				if(i+c < size)
					fprintf(stream, "%02x ", ptr[i+c]);
				else
					fputs("   ", stream);
		}
		
		for(c = 0; (c < width) && (i+c < size); c++) {
			/* check for 0D0A; if found, skip past and start a new line of output */
			if (nohex && (i+c+1 < size) && ptr[i+c]==0x0D && ptr[i+c+1]==0x0A) {
				i+=(c+2-width);
				break;
			}
			fprintf(stream, "%c",
					(ptr[i+c]>=0x20) && (ptr[i+c]<0x80)?ptr[i+c]:'.');
			/* check again for 0D0A, to avoid an extra \n if it's at width */
			if (nohex && (i+c+2 < size) && ptr[i+c+1]==0x0D && ptr[i+c+2]==0x0A) {
				i+=(c+3-width);
				break;
			}
		}
		fputc('\n', stream); /* newline */
	}
	fflush(stream);
}

static int my_trace(CURL *handle, curl_infotype type, char *data, size_t size, void *userp)
{
	const char *text;
	(void)handle; /* prevent compiler warning */
	
	switch (type) {
		case CURLINFO_TEXT:
			fprintf(stderr, "== Info: %s", data);
		default: /* in case a new one is introduced to shock us */
			return 0;
			
		case CURLINFO_HEADER_OUT:
			text = "=> Send header";
			break;
		case CURLINFO_DATA_OUT:
			text = "=> Send data";
			break;
		case CURLINFO_SSL_DATA_OUT:
			text = "=> Send SSL data";
			break;
		case CURLINFO_HEADER_IN:
			text = "<= Recv header";
			break;
		case CURLINFO_DATA_IN:
			text = "<= Recv data";
			break;
		case CURLINFO_SSL_DATA_IN:
			text = "<= Recv SSL data";
			break;
	}
	
	struct data *config = (struct data *)userp;
	
	if (config)
		dump(text, stdout, (unsigned char *)data, size, config->trace_ascii);
	else
		dump(text, stdout, (unsigned char *)data, size, 1);
	
	return 0;
}
