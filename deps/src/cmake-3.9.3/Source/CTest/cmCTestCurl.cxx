/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestCurl.h"

#include "cmCTest.h"
#include "cmSystemTools.h"

#include "cmConfigure.h"
#include <ostream>
#include <stdio.h>

cmCTestCurl::cmCTestCurl(cmCTest* ctest)
{
  this->CTest = ctest;
  this->SetProxyType();
  this->UseHttp10 = false;
  // In windows, this will init the winsock stuff
  ::curl_global_init(CURL_GLOBAL_ALL);
  // default is to verify https
  this->VerifyPeerOff = false;
  this->VerifyHostOff = false;
  this->Quiet = false;
  this->TimeOutSeconds = 0;
  this->Curl = curl_easy_init();
}

cmCTestCurl::~cmCTestCurl()
{
  ::curl_easy_cleanup(this->Curl);
  ::curl_global_cleanup();
}

std::string cmCTestCurl::Escape(std::string const& source)
{
  char* data1 = curl_easy_escape(this->Curl, source.c_str(), 0);
  std::string ret = data1;
  curl_free(data1);
  return ret;
}

namespace {
size_t curlWriteMemoryCallback(void* ptr, size_t size, size_t nmemb,
                               void* data)
{
  int realsize = (int)(size * nmemb);

  std::vector<char>* vec = static_cast<std::vector<char>*>(data);
  const char* chPtr = static_cast<char*>(ptr);
  vec->insert(vec->end(), chPtr, chPtr + realsize);
  return realsize;
}

size_t curlDebugCallback(CURL* /*unused*/, curl_infotype /*unused*/,
                         char* chPtr, size_t size, void* data)
{
  std::vector<char>* vec = static_cast<std::vector<char>*>(data);
  vec->insert(vec->end(), chPtr, chPtr + size);

  return size;
}
}

void cmCTestCurl::SetCurlOptions(std::vector<std::string> const& args)
{
  for (std::vector<std::string>::const_iterator i = args.begin();
       i != args.end(); ++i) {
    if (*i == "CURLOPT_SSL_VERIFYPEER_OFF") {
      this->VerifyPeerOff = true;
    }
    if (*i == "CURLOPT_SSL_VERIFYHOST_OFF") {
      this->VerifyHostOff = true;
    }
  }
}

bool cmCTestCurl::InitCurl()
{
  if (!this->Curl) {
    return false;
  }
  if (this->VerifyPeerOff) {
    curl_easy_setopt(this->Curl, CURLOPT_SSL_VERIFYPEER, 0);
  }
  if (this->VerifyHostOff) {
    curl_easy_setopt(this->Curl, CURLOPT_SSL_VERIFYHOST, 0);
  }
  if (!this->HTTPProxy.empty()) {
    curl_easy_setopt(this->Curl, CURLOPT_PROXY, this->HTTPProxy.c_str());
    curl_easy_setopt(this->Curl, CURLOPT_PROXYTYPE, this->HTTPProxyType);
    if (!this->HTTPProxyAuth.empty()) {
      curl_easy_setopt(this->Curl, CURLOPT_PROXYUSERPWD,
                       this->HTTPProxyAuth.c_str());
    }
  }
  if (this->UseHttp10) {
    curl_easy_setopt(this->Curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
  }
  // enable HTTP ERROR parsing
  curl_easy_setopt(this->Curl, CURLOPT_FAILONERROR, 1);

  // if there is little to no activity for too long stop submitting
  if (this->TimeOutSeconds) {
    curl_easy_setopt(this->Curl, CURLOPT_LOW_SPEED_LIMIT, 1);
    curl_easy_setopt(this->Curl, CURLOPT_LOW_SPEED_TIME, this->TimeOutSeconds);
  }

  return true;
}

bool cmCTestCurl::UploadFile(std::string const& local_file,
                             std::string const& url, std::string const& fields,
                             std::string& response)
{
  response = "";
  if (!this->InitCurl()) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "Initialization of curl failed");
    return false;
  }
  /* enable uploading */
  curl_easy_setopt(this->Curl, CURLOPT_UPLOAD, 1);

  /* HTTP PUT please */
  ::curl_easy_setopt(this->Curl, CURLOPT_PUT, 1);
  ::curl_easy_setopt(this->Curl, CURLOPT_VERBOSE, 1);

  FILE* ftpfile = cmsys::SystemTools::Fopen(local_file, "rb");
  if (!ftpfile) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Could not open file for upload: " << local_file << "\n");
    return false;
  }
  // set the url
  std::string upload_url = url;
  upload_url += "?";
  upload_url += fields;
  ::curl_easy_setopt(this->Curl, CURLOPT_URL, upload_url.c_str());
  // now specify which file to upload
  ::curl_easy_setopt(this->Curl, CURLOPT_INFILE, ftpfile);
  unsigned long filelen = cmSystemTools::FileLength(local_file);
  // and give the size of the upload (optional)
  ::curl_easy_setopt(this->Curl, CURLOPT_INFILESIZE,
                     static_cast<long>(filelen));
  ::curl_easy_setopt(this->Curl, CURLOPT_WRITEFUNCTION,
                     curlWriteMemoryCallback);
  ::curl_easy_setopt(this->Curl, CURLOPT_DEBUGFUNCTION, curlDebugCallback);
  // Set Content-Type to satisfy fussy modsecurity rules.
  struct curl_slist* headers =
    ::curl_slist_append(CM_NULLPTR, "Content-Type: text/xml");
  // Add any additional headers that the user specified.
  for (std::vector<std::string>::const_iterator h = this->HttpHeaders.begin();
       h != this->HttpHeaders.end(); ++h) {
    cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT,
                       "   Add HTTP Header: \"" << *h << "\"" << std::endl,
                       this->Quiet);
    headers = ::curl_slist_append(headers, h->c_str());
  }
  ::curl_easy_setopt(this->Curl, CURLOPT_HTTPHEADER, headers);
  std::vector<char> responseData;
  std::vector<char> debugData;
  ::curl_easy_setopt(this->Curl, CURLOPT_FILE, (void*)&responseData);
  ::curl_easy_setopt(this->Curl, CURLOPT_DEBUGDATA, (void*)&debugData);
  ::curl_easy_setopt(this->Curl, CURLOPT_FAILONERROR, 1);
  // Now run off and do what you've been told!
  ::curl_easy_perform(this->Curl);
  ::fclose(ftpfile);
  ::curl_easy_setopt(this->Curl, CURLOPT_HTTPHEADER, NULL);
  ::curl_slist_free_all(headers);

  if (!responseData.empty()) {
    response = std::string(responseData.begin(), responseData.end());
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Curl response: [" << response << "]\n", this->Quiet);
  }
  std::string curlDebug;
  if (!debugData.empty()) {
    curlDebug = std::string(debugData.begin(), debugData.end());
    cmCTestOptionalLog(this->CTest, DEBUG,
                       "Curl debug: [" << curlDebug << "]\n", this->Quiet);
  }
  if (response.empty()) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "No response from server.\n"
                 << curlDebug);
    return false;
  }
  return true;
}

bool cmCTestCurl::HttpRequest(std::string const& url,
                              std::string const& fields, std::string& response)
{
  response = "";
  cmCTestOptionalLog(this->CTest, DEBUG, "HttpRequest\n"
                       << "url: " << url << "\n"
                       << "fields " << fields << "\n",
                     this->Quiet);
  if (!this->InitCurl()) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "Initialization of curl failed");
    return false;
  }
  curl_easy_setopt(this->Curl, CURLOPT_POST, 1);
  curl_easy_setopt(this->Curl, CURLOPT_POSTFIELDS, fields.c_str());
  ::curl_easy_setopt(this->Curl, CURLOPT_URL, url.c_str());
  ::curl_easy_setopt(this->Curl, CURLOPT_FOLLOWLOCATION, 1);
  // set response options
  ::curl_easy_setopt(this->Curl, CURLOPT_WRITEFUNCTION,
                     curlWriteMemoryCallback);
  ::curl_easy_setopt(this->Curl, CURLOPT_DEBUGFUNCTION, curlDebugCallback);
  std::vector<char> responseData;
  std::vector<char> debugData;
  ::curl_easy_setopt(this->Curl, CURLOPT_FILE, (void*)&responseData);
  ::curl_easy_setopt(this->Curl, CURLOPT_DEBUGDATA, (void*)&debugData);
  ::curl_easy_setopt(this->Curl, CURLOPT_FAILONERROR, 1);

  // Add headers if any were specified.
  struct curl_slist* headers = CM_NULLPTR;
  if (!this->HttpHeaders.empty()) {
    for (std::vector<std::string>::const_iterator h =
           this->HttpHeaders.begin();
         h != this->HttpHeaders.end(); ++h) {
      cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT,
                         "   Add HTTP Header: \"" << *h << "\"" << std::endl,
                         this->Quiet);
      headers = ::curl_slist_append(headers, h->c_str());
    }
  }

  ::curl_easy_setopt(this->Curl, CURLOPT_HTTPHEADER, headers);
  CURLcode res = ::curl_easy_perform(this->Curl);
  ::curl_slist_free_all(headers);

  if (!responseData.empty()) {
    response = std::string(responseData.begin(), responseData.end());
    cmCTestOptionalLog(this->CTest, DEBUG,
                       "Curl response: [" << response << "]\n", this->Quiet);
  }
  if (!debugData.empty()) {
    std::string curlDebug = std::string(debugData.begin(), debugData.end());
    cmCTestOptionalLog(this->CTest, DEBUG,
                       "Curl debug: [" << curlDebug << "]\n", this->Quiet);
  }
  cmCTestOptionalLog(this->CTest, DEBUG, "Curl res: " << res << "\n",
                     this->Quiet);
  return (res == 0);
}

void cmCTestCurl::SetProxyType()
{
  this->HTTPProxy = "";
  // this is the default
  this->HTTPProxyType = CURLPROXY_HTTP;
  this->HTTPProxyAuth = "";
  if (cmSystemTools::GetEnv("HTTP_PROXY", this->HTTPProxy)) {
    std::string port;
    if (cmSystemTools::GetEnv("HTTP_PROXY_PORT", port)) {
      this->HTTPProxy += ":";
      this->HTTPProxy += port;
    }
    std::string type;
    if (cmSystemTools::GetEnv("HTTP_PROXY_TYPE", type)) {
      // HTTP/SOCKS4/SOCKS5
      if (type == "HTTP") {
        this->HTTPProxyType = CURLPROXY_HTTP;
      } else if (type == "SOCKS4") {
        this->HTTPProxyType = CURLPROXY_SOCKS4;
      } else if (type == "SOCKS5") {
        this->HTTPProxyType = CURLPROXY_SOCKS5;
      }
    }
    cmSystemTools::GetEnv("HTTP_PROXY_USER", this->HTTPProxyAuth);
    std::string passwd;
    if (cmSystemTools::GetEnv("HTTP_PROXY_PASSWD", passwd)) {
      this->HTTPProxyAuth += ":";
      this->HTTPProxyAuth += passwd;
    }
  }
}
