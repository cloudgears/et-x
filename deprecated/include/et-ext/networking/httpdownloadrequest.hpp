/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/networking/httpasynchronousrequest.hpp>

namespace et {
class HTTPDownloadRequest : public HTTPRequest {
 public:
  ET_DECLARE_POINTER(HTTPDownloadRequest)

 public:
  HTTPDownloadRequest(const std::string& url, const std::string& file)
    : HTTPRequest(url)
    , _targetFile(file) {}

  void downloadSynchronously() {
    perform();

    auto requestResponse = response();

    if (succeeded() && (requestResponse->responseCode() == ResponseSuccessful)) requestResponse->data().writeToFile(_targetFile, true);
  }

  template <typename F>
  void downloadAsynchronously(F callback) {
    HTTPDownloadRequest::Pointer holder(this);
    Invocation([this, callback, holder]() mutable {
      downloadSynchronously();
      callback(response());
      holder.reset(nullptr);
    }).invokeInRunLoop(sharedHTTPRequestsThread().runLoop());
  }

 private:
  void perform() {
    HTTPRequest::perform();
  }

 private:
  std::string _targetFile;
};
}  // namespace et