#include "LoadedCommand.h"
#include <stdio.h>

int testSizeOf(int s1, int s2)
{
  return s1 - s2;
}

int main ()
{
  int ret = 0;

#if !defined( ADDED_DEFINITION )
  printf("Should have ADDED_DEFINITION defined\n");
  ret= 1;
#endif
#if !defined(CMAKE_IS_FUN)
  printf("Loaded Command was not built with CMAKE_IS_FUN: failed.\n");
  ret = 1;
#endif
  if(testSizeOf(SIZEOF_CHAR, sizeof(char)))
    {
    printf("Size of char is broken.\n");
    ret = 1;
    }
  if(testSizeOf(SIZEOF_SHORT, sizeof(short)))
    {
    printf("Size of short is broken.\n");
    ret = 1;
    }
  return ret;
}
