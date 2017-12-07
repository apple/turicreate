#ifndef TEST_DEFINE
#error Expected definition TEST_DEFINE
#endif

#ifndef NEEDS_ESCAPE
#error Expected definition NEEDS_ESCAPE
#endif

#ifdef DO_GNU_TESTS
#ifndef TEST_DEFINE_GNU
#error Expected definition TEST_DEFINE_GNU
#endif
#endif

#include <string.h>

int main()
{
  return (strcmp(NEEDS_ESCAPE, "E$CAPE") == 0
#ifdef TEST_OCTOTHORPE
          && strcmp(TEST_OCTOTHORPE, "#") == 0
#endif
          &&
          strcmp(EXPECTED_C_COMPILER_VERSION, TEST_C_COMPILER_VERSION) == 0 &&
          strcmp(EXPECTED_CXX_COMPILER_VERSION, TEST_CXX_COMPILER_VERSION) ==
            0 &&
          TEST_C_COMPILER_VERSION_EQUALITY == 1 &&
          TEST_CXX_COMPILER_VERSION_EQUALITY == 1)
    ? 0
    : 1;
}
