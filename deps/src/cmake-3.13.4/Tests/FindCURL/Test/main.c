#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
  struct curl_slist* slist;

  curl_global_init(0);

  slist = curl_slist_append(NULL, "CMake");
  curl_slist_free_all(slist);

  curl_global_cleanup();

  return 0;
}
