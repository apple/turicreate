
# Import targets from the exported build tree.
include(${Import_BINARY_DIR}/../Export/ExportInterfaceBuildTree.cmake)

# Import targets from the exported install tree.
include(${CMAKE_INSTALL_PREFIX}/lib/exp/expInterface.cmake)

add_library(define_iface INTERFACE)
set_property(TARGET define_iface PROPERTY
  INTERFACE_COMPILE_DEFINITIONS DEFINE_IFACE_DEFINE)

add_executable(headeronlytest_bld headeronlytest.cpp)
target_link_libraries(headeronlytest_bld bld::headeronly)

set_property(TARGET bld::sharediface APPEND PROPERTY INTERFACE_LINK_LIBRARIES define_iface)

add_executable(interfacetest_bld interfacetest.cpp)
target_link_libraries(interfacetest_bld bld::sharediface)

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

macro(do_try_compile prefix)

  set(CMAKE_REQUIRED_LIBRARIES ${prefix}::headeronly)
  check_cxx_source_compiles(
    "
  #include \"headeronly.h\"

  #ifndef HEADERONLY_DEFINE
  #error Expected HEADERONLY_DEFINE
  #endif

  int main(int,char**)
  {
    HeaderOnly ho;
    return ho.foo();
  }
  " ${prefix}IFACE_TRY_COMPILE)

  if(NOT ${prefix}IFACE_TRY_COMPILE)
    message(SEND_ERROR "${prefix} try_compile with IMPORTED INTERFACE target failed!\n\n${OUTPUT}")
  endif()

  if (";${CMAKE_C_COMPILE_FEATURES};" MATCHES ";c_restrict;")
    set(CMAKE_REQUIRED_LIBRARIES ${prefix}::use_c_restrict)
    check_c_source_compiles(
      "
    int foo(int * restrict a, int * restrict b)
    {
      (void)a;
      (void)b;
      return 0;
    }
    int main()
    {
      return 0;
    }
    " ${prefix}IMPORTED_IFACE_C_RESTRICT)

    if(NOT ${prefix}IMPORTED_IFACE_C_RESTRICT)
      message(SEND_ERROR "${prefix} try_compile with IMPORTED INTERFACE target failed!\n\n${OUTPUT}")
    endif()
  endif()
  if (";${CMAKE_CXX_COMPILE_FEATURES};" MATCHES ";cxx_auto_type;")
    set(CMAKE_REQUIRED_LIBRARIES ${prefix}::use_auto_type)
    check_cxx_source_compiles(
      "
    int main(int,char**)
    {
      auto value = 0;
      return value;
    }
    " ${prefix}IMPORTED_IFACE_AUTO_TYPE)

    if(NOT ${prefix}IMPORTED_IFACE_AUTO_TYPE)
      message(SEND_ERROR "${prefix} try_compile with IMPORTED INTERFACE target failed!\n\n${OUTPUT}")
    endif()
  endif()

endmacro()

do_try_compile(bld)

add_executable(source_target_test_bld source_target_test.cpp)
target_link_libraries(source_target_test_bld bld::source_target)
target_compile_definitions(source_target_test_bld PRIVATE USE_FROM_BUILD_DIR)

add_executable(source_target_test_exp source_target_test.cpp)
target_link_libraries(source_target_test_exp exp::source_target)
target_compile_definitions(source_target_test_exp PRIVATE USE_FROM_INSTALL_DIR)

add_executable(headeronlytest_exp headeronlytest.cpp)
target_link_libraries(headeronlytest_exp exp::headeronly)

set_property(TARGET exp::sharediface APPEND PROPERTY INTERFACE_LINK_LIBRARIES define_iface)

add_executable(interfacetest_exp interfacetest.cpp)
target_link_libraries(interfacetest_exp exp::sharediface)

do_try_compile(exp)

foreach(ns exp bld)
  get_property(defs TARGET ${ns}::cmakeonly PROPERTY INTERFACE_COMPILE_DEFINITIONS)
  if(NOT defs STREQUAL [[DEF="\"\$\B"]])
    message(SEND_ERROR
      "${ns}::cmakeonly property INTERFACE_COMPILE_DEFINITIONS is:\n"
      " ${defs}\n"
      "not\n"
      " " [[DEF="\"\$\B"]] "\n")
  endif()
endforeach()
