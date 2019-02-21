#include <expat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
  XML_Expat_Version expat_version;
  char expat_version_string[16];

  expat_version = XML_ExpatVersionInfo();

  snprintf(expat_version_string, 16, "%i.%i.%i", expat_version.major,
           expat_version.minor, expat_version.micro);

  if (strcmp(expat_version_string, CMAKE_EXPECTED_EXPAT_VERSION) != 0) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
