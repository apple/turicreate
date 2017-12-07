
cmake_minimum_required(VERSION 2.8)

find_package(testLibRequired 2.5 REQUIRED)

include(CheckCXXSourceCompiles)

set(CMAKE_REQUIRED_LIBRARIES Req::testSharedLibRequiredUser)
check_cxx_source_compiles(
  "
#include \"testSharedLibRequiredUser.h\"
int main(int argc, char **argv)
{
  TestSharedLibRequiredUser user;
  return user.foo();
}
" SHARED_LIB_DEPENDS)

if(NOT SHARED_LIB_DEPENDS)
  message(SEND_ERROR "try_compile with IMPORTED targets failed!\n\n${OUTPUT}")
endif()

set(CMAKE_REQUIRED_LIBRARIES Req::testSharedLibRequiredUser2)
check_cxx_source_compiles(
  "
#include \"testSharedLibRequiredUser2.h\"
int main(int argc, char **argv)
{
  TestSharedLibRequiredUser2 user;
  return user.foo().foo();
}
" SHARED_LIB_DEPENDS2)

if(NOT SHARED_LIB_DEPENDS2)
  message(SEND_ERROR "try_compile with IMPORTED targets failed!\n\n${OUTPUT}")
endif()
