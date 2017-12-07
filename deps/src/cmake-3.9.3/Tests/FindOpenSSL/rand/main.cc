#include <openssl/rand.h>

int main()
{
  // return value
  int retval = 1;

  // bytes buffer
  unsigned char buf[1024];

  // random bytes
  int rezval = RAND_bytes(buf, sizeof(buf)); /* 1 succes, 0 otherwise */

  // check result
  if (rezval == 1) {
    retval = 0;
  }

  // return code
  return retval;
}
