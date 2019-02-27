
#ifndef BOOL_PROP1
#  error Expected BOOL_PROP1
#endif

#ifndef BOOL_PROP2
#  error Expected BOOL_PROP2
#endif

#ifndef BOOL_PROP3
#  error Expected BOOL_PROP3
#endif

#ifndef STRING_PROP1
#  error Expected STRING_PROP1
#endif

#ifndef STRING_PROP2
#  error Expected STRING_PROP2
#endif

#ifndef STRING_PROP3
#  error Expected STRING_PROP3
#endif

template <bool test>
struct CMakeStaticAssert;

template <>
struct CMakeStaticAssert<true>
{
};

enum
{
  NumericMaxTest1 = sizeof(CMakeStaticAssert<NUMBER_MAX_PROP1 == 100>),
  NumericMaxTest2 = sizeof(CMakeStaticAssert<NUMBER_MAX_PROP2 == 250>),
  NumericMinTest1 = sizeof(CMakeStaticAssert<NUMBER_MIN_PROP1 == 50>),
  NumericMinTest2 = sizeof(CMakeStaticAssert<NUMBER_MIN_PROP2 == 200>),
  NumericMinTest3 = sizeof(CMakeStaticAssert<NUMBER_MIN_PROP3 == 0xA>),
  NumericMinTest4 = sizeof(CMakeStaticAssert<NUMBER_MIN_PROP4 == 0x10>)
};

#include "iface2.h"

int foo();
#ifdef _WIN32
__declspec(dllimport)
#endif
  int bar();

int main(int argc, char** argv)
{
  Iface2 if2;
  return if2.foo() + foo() + bar();
}
