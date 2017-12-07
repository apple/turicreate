

#include "testSharedLibDepends.h"

#ifdef CHECK_PIC_WORKS
#if defined(__ELF__) && !defined(__PIC__) && !defined(__PIE__)
#error Expected by INTERFACE_POSITION_INDEPENDENT_CODE property of dependency
#endif
#endif

#ifndef PIC_PROPERTY_IS_ON
#error Expected PIC_PROPERTY_IS_ON
#endif

#ifndef CUSTOM_PROPERTY_IS_ON
#error Expected CUSTOM_PROPERTY_IS_ON
#endif

#ifndef CUSTOM_STRING_IS_MATCH
#error Expected CUSTOM_STRING_IS_MATCH
#endif

#ifdef TEST_SUBDIR_LIB
#include "renamed.h"
#include "subdir.h"
#endif

#ifdef DO_GNU_TESTS
#ifndef CUSTOM_COMPILE_OPTION
#error Expected CUSTOM_COMPILE_OPTION
#endif
#endif

int main(int, char**)
{
  TestSharedLibDepends dep;
  TestSharedLibRequired req;

#ifdef TEST_SUBDIR_LIB
  SubDirObject sdo;
  Renamed ren;
#endif

  return dep.foo() + req.foo()
#ifdef TEST_SUBDIR_LIB
    + sdo.foo() + ren.foo()
#endif
    ;
}
