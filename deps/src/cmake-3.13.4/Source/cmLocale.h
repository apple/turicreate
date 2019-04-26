/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmLocale_h
#define cmLocale_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <locale.h>
#include <string>

class cmLocaleRAII
{
  CM_DISABLE_COPY(cmLocaleRAII)

public:
  cmLocaleRAII()
    : OldLocale(setlocale(LC_CTYPE, nullptr))
  {
    setlocale(LC_CTYPE, "");
  }
  ~cmLocaleRAII() { setlocale(LC_CTYPE, this->OldLocale.c_str()); }

private:
  std::string OldLocale;
};

#endif
