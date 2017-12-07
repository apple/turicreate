#include <assert.h>
#include <tiffio.h>

int main()
{
  /* Without any TIFF file to open, test that the call fails as
     expected.  This tests that linking worked. */
  TIFF* tiff = TIFFOpen("invalid.tiff", "r");
  assert(!tiff);

  return 0;
}
