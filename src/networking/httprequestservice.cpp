//
//  httprequestservice.cpp
//  Jewellery Lab
//
//  Created by Sergey Reznik on 25/5/2014.
//  Copyright (c) 2014 Sergey Reznik. All rights reserved.
//

#include <libcurl/curl.h>
#include <et-ext/networking/httprequestservice.h>

using namespace et;

static size_t dataRead = 0;

size_t my_write(const void* ptr, size_t s1, size_t s2, FILE* f)
{
	dataRead += s1*s2;
	return s1*s2;
}

void HTTPRequestService::test()
{
	CURL* curl = curl_easy_init();
	
	/*
	curl_easy_setopt(curl, CURLOPT_FILE, request.ptr());
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progessCallback);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, request.ptr());
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
	*/
	
	curl_easy_setopt(curl, CURLOPT_USERNAME, "reznik");
	curl_easy_setopt(curl, CURLOPT_PASSWORD, "sergey");
	curl_easy_setopt(curl, CURLOPT_URL, "http://music.dorokhov.net/api/artists");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_write);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(curl, CURLOPT_PORT, 80);
	
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	
	log::info("%zu bytes read", dataRead);
}
