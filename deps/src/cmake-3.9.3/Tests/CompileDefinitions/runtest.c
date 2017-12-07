#include <ctype.h>
#include <stdio.h>
#include <string.h>

#ifndef BUILD_CONFIG_NAME
#error "BUILD_CONFIG_NAME not defined!"
#endif

int main()
{
  char build_config_name[] = BUILD_CONFIG_NAME;
  char* c;
  for (c = build_config_name; *c; ++c) {
    *c = (char)tolower((int)*c);
  }
  fprintf(stderr, "build_config_name=\"%s\"\n", build_config_name);
#ifdef TEST_CONFIG_DEBUG
  if (strcmp(build_config_name, "debug") != 0) {
    fprintf(stderr, "build_config_name is not \"debug\"\n");
    return 1;
  }
#endif
#ifdef TEST_CONFIG_RELEASE
  if (strcmp(build_config_name, "release") != 0) {
    fprintf(stderr, "build_config_name is not \"release\"\n");
    return 1;
  }
#endif
#ifdef TEST_CONFIG_MINSIZEREL
  if (strcmp(build_config_name, "minsizerel") != 0) {
    fprintf(stderr, "build_config_name is not \"minsizerel\"\n");
    return 1;
  }
#endif
#ifdef TEST_CONFIG_RELWITHDEBINFO
  if (strcmp(build_config_name, "relwithdebinfo") != 0) {
    fprintf(stderr, "build_config_name is not \"relwithdebinfo\"\n");
    return 1;
  }
#endif
  return 0;
}
