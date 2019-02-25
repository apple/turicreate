/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#include "cmConfigure.h" // IWYU pragma: keep

#include <sstream>
#include <stdio.h>
#include <string>

#include "cmXMLSafe.h"

struct test_pair
{
  const char* in;
  const char* out;
};

static test_pair const pairs[] = {
  { "copyright \xC2\xA9", "copyright \xC2\xA9" },
  { "form-feed \f", "form-feed [NON-XML-CHAR-0xC]" },
  { "angles <>", "angles &lt;&gt;" },
  { "ampersand &", "ampersand &amp;" },
  { "bad-byte \x80", "bad-byte [NON-UTF-8-BYTE-0x80]" },
  { nullptr, nullptr }
};

int testXMLSafe(int /*unused*/, char* /*unused*/ [])
{
  int result = 0;
  for (test_pair const* p = pairs; p->in; ++p) {
    cmXMLSafe xs(p->in);
    std::ostringstream oss;
    oss << xs;
    std::string out = oss.str();
    if (out != p->out) {
      printf("expected [%s], got [%s]\n", p->out, out.c_str());
      result = 1;
    }
  }
  return result;
}
