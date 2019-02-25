#include <ft2build.h>
#include <stdlib.h>
#include FT_FREETYPE_H
#include <string.h>

int main()
{
  FT_Library library;
  FT_Error error;

  error = FT_Init_FreeType(&library);
  if (error) {
    return EXIT_FAILURE;
  }

  FT_Int major = 0;
  FT_Int minor = 0;
  FT_Int patch = 0;
  FT_Library_Version(library, &major, &minor, &patch);

  char ft_version_string[16];
  snprintf(ft_version_string, 16, "%i.%i.%i", major, minor, patch);

  if (strcmp(ft_version_string, CMAKE_EXPECTED_FREETYPE_VERSION) != 0) {
    return EXIT_FAILURE;
  }

  error = FT_Done_FreeType(library);
  if (error) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
