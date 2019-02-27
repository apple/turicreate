
set(CMAKE_POSITION_INDEPENDENT_CODE True)

add_executable(test_target_executable_global
  "${CMAKE_CURRENT_SOURCE_DIR}/../pic_main.cpp"
)

add_library(test_target_shared_library_global
  SHARED "${CMAKE_CURRENT_SOURCE_DIR}/../pic_lib.cpp"
)
set_target_properties(test_target_shared_library_global
  PROPERTIES DEFINE_SYMBOL PIC_TEST_BUILD_DLL
)

add_library(test_target_static_library_global
  STATIC "${CMAKE_CURRENT_SOURCE_DIR}/../pic_lib.cpp"
)
set_target_properties(test_target_static_library_global
  PROPERTIES COMPILE_DEFINITIONS PIC_TEST_STATIC_BUILD
)


file(READ
  "${CMAKE_CURRENT_SOURCE_DIR}/../pic_test.h"
  PIC_HEADER_CONTENT
)

check_cxx_source_compiles(
  "
${PIC_HEADER_CONTENT}
int main(int,char**) { return 0; }\n"
  PIC_TRY_COMPILE_RESULT
)

if (NOT PIC_TRY_COMPILE_RESULT)
  message(SEND_ERROR "TRY_COMPILE with content requiring __PIC__ failed. ${OUTPUT}")
endif()
