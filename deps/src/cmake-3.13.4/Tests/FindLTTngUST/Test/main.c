#include <assert.h>
#include <lttng/ust-version.h>
#include <stdio.h>
#include <string.h>

#ifdef CMAKE_LTTNGUST_HAS_TRACEF
#  include <lttng/tracef.h>
#endif

#ifdef CMAKE_LTTNGUST_HAS_TRACELOG
#  include <lttng/tracelog.h>
#endif

int main(void)
{
  char lttng_version_string[16];

  snprintf(lttng_version_string, 16, "%u.%u.%u", LTTNG_UST_MAJOR_VERSION,
           LTTNG_UST_MINOR_VERSION, LTTNG_UST_PATCHLEVEL_VERSION);
  assert(!strcmp(lttng_version_string, CMAKE_EXPECTED_LTTNGUST_VERSION));

#ifdef CMAKE_LTTNGUST_HAS_TRACEF
  tracef("calling tracef()! %d %s", -23, CMAKE_EXPECTED_LTTNGUST_VERSION);
#endif

#ifdef CMAKE_LTTNGUST_HAS_TRACELOG
  tracelog(TRACE_WARNING, "calling tracelog()! %d", 17);
#endif

  return 0;
}
