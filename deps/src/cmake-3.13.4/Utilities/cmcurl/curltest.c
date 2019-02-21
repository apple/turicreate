#include "curl/curl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test_curl(const char* url)
{
  CURL* curl;
  CURLcode r;
  char proxy[1024];
  int proxy_type = 0;

  if (getenv("HTTP_PROXY")) {
    proxy_type = 1;
    if (getenv("HTTP_PROXY_PORT")) {
      sprintf(proxy, "%s:%s", getenv("HTTP_PROXY"), getenv("HTTP_PROXY_PORT"));
    } else {
      sprintf(proxy, "%s", getenv("HTTP_PROXY"));
    }
    if (getenv("HTTP_PROXY_TYPE")) {
      /* HTTP/SOCKS4/SOCKS5 */
      if (strcmp(getenv("HTTP_PROXY_TYPE"), "HTTP") == 0) {
        proxy_type = 1;
      } else if (strcmp(getenv("HTTP_PROXY_TYPE"), "SOCKS4") == 0) {
        proxy_type = 2;
      } else if (strcmp(getenv("HTTP_PROXY_TYPE"), "SOCKS5") == 0) {
        proxy_type = 3;
      }
    }
  }

  curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "curl_easy_init failed\n");
    return 1;
  }

  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
  curl_easy_setopt(curl, CURLOPT_HEADER, 1);

  if (proxy_type > 0) {
    curl_easy_setopt(curl, CURLOPT_PROXY, proxy);
    switch (proxy_type) {
      case 2:
        curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS4);
        break;
      case 3:
        curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5);
        break;
      default:
        curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
    }
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  r = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (r != CURLE_OK) {
    fprintf(stderr, "error: fetching '%s' failed: %s\n", url,
            curl_easy_strerror(r));
    return 1;
  }

  return 0;
}

int main(int argc, const char* argv[])
{
  int r;
  curl_global_init(CURL_GLOBAL_DEFAULT);
  if (argc == 2) {
    r = test_curl(argv[1]);
  } else {
    fprintf(stderr, "error: no URL given as first argument\n");
    r = 1;
  }
  curl_global_cleanup();
  return r;
}
