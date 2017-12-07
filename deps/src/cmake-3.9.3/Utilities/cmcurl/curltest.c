/* Prevent warnings on Visual Studio */
struct _RPC_ASYNC_STATE;

#include "curl/curl.h"
#include <stdlib.h>
#include <string.h>

int GetFtpFile(void)
{
  int retVal = 0;
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();
  if(curl) 
    {
    /* Get curl 7.9.2 from sunet.se's FTP site: */
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl, CURLOPT_HEADER, 1);
    curl_easy_setopt(curl, CURLOPT_URL,
                     "ftp://public.kitware.com/pub/cmake/cygwin/setup.hint");
    res = curl_easy_perform(curl);
    if ( res != 0 )
      {
      printf("Error fetching: ftp://public.kitware.com/pub/cmake/cygwin/setup.hint\n");
      retVal = 1;
      }

    /* always cleanup */
    curl_easy_cleanup(curl);
    }
  else
    {
    printf("Cannot create curl object\n");
    retVal = 1;
    }
  return retVal;
}

int GetWebFiles(char *url1, char *url2)
{
  int retVal = 0;
  CURL *curl;
  CURLcode res;

  char proxy[1024];
  int proxy_type = 0;

  if ( getenv("HTTP_PROXY") )
    {
    proxy_type = 1;
    if (getenv("HTTP_PROXY_PORT") )
      {
      sprintf(proxy, "%s:%s", getenv("HTTP_PROXY"), getenv("HTTP_PROXY_PORT"));
      }
    else
      {
      sprintf(proxy, "%s", getenv("HTTP_PROXY"));
      }
    if ( getenv("HTTP_PROXY_TYPE") )
      {
      /* HTTP/SOCKS4/SOCKS5 */
      if ( strcmp(getenv("HTTP_PROXY_TYPE"), "HTTP") == 0 )
        {
        proxy_type = 1;
        }
      else if ( strcmp(getenv("HTTP_PROXY_TYPE"), "SOCKS4") == 0 )
        {
        proxy_type = 2;
        }
      else if ( strcmp(getenv("HTTP_PROXY_TYPE"), "SOCKS5") == 0 )
        {
        proxy_type = 3;
        }
      }
    }

  curl = curl_easy_init();
  if(curl) 
    {
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl, CURLOPT_HEADER, 1);

    /* Using proxy */
    if ( proxy_type > 0 )
      {
      curl_easy_setopt(curl, CURLOPT_PROXY, proxy); 
      switch (proxy_type)
        {
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

    /* get the first document */
    curl_easy_setopt(curl, CURLOPT_URL, url1);
    res = curl_easy_perform(curl);
    if ( res != 0 )
      {
      printf("Error fetching: %s\n", url1);
      retVal = 1;
      }

    /* get another document from the same server using the same
       connection */
    /* avoid warnings about url2 since below block is commented out: */
    (void) url2;
    /*
      curl_easy_setopt(curl, CURLOPT_URL, url2);
      res = curl_easy_perform(curl);
      if ( res != 0 )
      {
      printf("Error fetching: %s\n", url2);
      retVal = 1;
      }
    */

    /* always cleanup */
    curl_easy_cleanup(curl);
    }
  else
    {
    printf("Cannot create curl object\n");
    retVal = 1;
    }

  return retVal;
}


int main(int argc, char **argv)
{
  int retVal = 0;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  if(argc>1)
    {
    retVal += GetWebFiles(argv[1], 0);
    }
  else
    {
    printf("error: first argument should be a url to download\n");
    retVal = 1;
    }

  /* Do not check the output of FTP socks5 cannot handle FTP yet */
  /* GetFtpFile(); */
  /* do not test ftp right now because we don't enable that port */

  curl_global_cleanup();

  return retVal;
}
