//
//  httprequestservice.h
//  Jewellery Lab
//
//  Created by Sergey Reznik on 25/5/2014.
//  Copyright (c) 2014 Sergey Reznik. All rights reserved.
//

#pragma once

#include <et/core/singleton.h>
#include <et-ext/networking/httprequest.h>

namespace et
{
	class HTTPRequestService : public Singleton<HTTPRequestService>
	{
	public:
		void test();
		
	private:
		ET_SINGLETON_CONSTRUCTORS(HTTPRequestService)
	};
}