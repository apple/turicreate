#include <stdio.h>
#include <test.h>
#include <test_i.c>

int main(int argc, char** argv)
{
  IID libid = LIBID_CMakeMidlTestLib;
  CLSID clsid = CLSID_CMakeMidlTest;
  IID iid = IID_ICMakeMidlTest;

  printf("Running '%s'\n", argv[0]);
  printf("  libid starts with '0x%08lx'\n", (long)libid.Data1);
  printf("  clsid starts with '0x%08lx'\n", (long)clsid.Data1);
  printf("    iid starts with '0x%08lx'\n", (long)iid.Data1);

  return 0;
}
