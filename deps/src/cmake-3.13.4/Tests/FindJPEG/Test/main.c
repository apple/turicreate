#include <assert.h>
#include <stdio.h>

#include <jpeglib.h>

int main()
{
  /* Without any JPEG file to open, test that the call fails as
     expected.  This tests that linking worked. */
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  return 0;
}
