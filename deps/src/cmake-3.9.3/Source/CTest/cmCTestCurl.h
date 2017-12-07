/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestCurl_h
#define cmCTestCurl_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cm_curl.h"
#include <string>
#include <vector>

class cmCTest;

class cmCTestCurl
{
public:
  cmCTestCurl(cmCTest*);
  ~cmCTestCurl();
  bool UploadFile(std::string const& url, std::string const& file,
                  std::string const& fields, std::string& response);
  bool HttpRequest(std::string const& url, std::string const& fields,
                   std::string& response);
  // currently only supports CURLOPT_SSL_VERIFYPEER_OFF
  // and CURLOPT_SSL_VERIFYHOST_OFF
  void SetCurlOptions(std::vector<std::string> const& args);
  void SetHttpHeaders(std::vector<std::string> const& v)
  {
    this->HttpHeaders = v;
  }
  void SetUseHttp10On() { this->UseHttp10 = true; }
  void SetTimeOutSeconds(int s) { this->TimeOutSeconds = s; }
  void SetQuiet(bool b) { this->Quiet = b; }
  std::string Escape(std::string const& source);

protected:
  void SetProxyType();
  bool InitCurl();

private:
  cmCTest* CTest;
  CURL* Curl;
  std::vector<std::string> HttpHeaders;
  std::string HTTPProxyAuth;
  std::string HTTPProxy;
  curl_proxytype HTTPProxyType;
  bool VerifyHostOff;
  bool VerifyPeerOff;
  bool UseHttp10;
  bool Quiet;
  int TimeOutSeconds;
};

#endif
