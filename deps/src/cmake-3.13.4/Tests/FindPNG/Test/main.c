#include <assert.h>
#include <png.h>
#include <string.h>

int main()
{
  png_uint_32 png_version;
  char png_version_string[16];

  png_version = png_access_version_number();

  snprintf(png_version_string, 16, "%i.%i.%i", png_version / 10000,
           (png_version % 10000) / 100, png_version % 100);

  assert(strcmp(png_version_string, CMAKE_EXPECTED_PNG_VERSION) == 0);

  return 0;
}
