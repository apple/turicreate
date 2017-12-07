#include <unicode/uclean.h>
#include <unicode/ustring.h>
#include <unicode/utypes.h>

#include <unicode/ucal.h>
#include <unicode/ucnv.h>
#include <unicode/udat.h>

int main()
{
  UConverter* cnv = 0;
  UErrorCode status = U_ZERO_ERROR;
  ucnv_open(NULL, &status);

  UChar uchars[100];
  const char* chars = "Test";
  if (cnv && U_SUCCESS(status)) {
    int32_t len = ucnv_toUChars(cnv, uchars, 100, chars, -1, &status);
  }

  ucnv_close(cnv);
  u_cleanup();
  return (U_FAILURE(status) ? 1 : 0);
}
