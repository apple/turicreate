
#ifndef IFACE_DEFINE
#error Expected IFACE_DEFINE
#endif

#include "iface_header.h"

#ifndef IFACE_HEADER_SRCDIR
#error Expected IFACE_HEADER_SRCDIR
#endif

#include "iface_header_builddir.h"

#ifndef IFACE_HEADER_BUILDDIR
#error Expected IFACE_HEADER_BUILDDIR
#endif

extern int obj();
extern int sub();
extern int item();

int main(int, char**)
{
  return obj() + sub() + item();
}
