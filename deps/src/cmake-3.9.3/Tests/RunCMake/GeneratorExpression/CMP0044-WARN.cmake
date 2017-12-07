
project(CMP0044-WARN)

string(TOLOWER ${CMAKE_C_COMPILER_ID} lc_test)
if (lc_test STREQUAL CMAKE_C_COMPILER_ID)
  string(TOUPPER ${CMAKE_C_COMPILER_ID} lc_test)
  if (lc_test STREQUAL CMAKE_C_COMPILER_ID)
    message(SEND_ERROR "Try harder.")
  endif()
endif()

add_library(cmp0044-check empty.c)
target_compile_definitions(cmp0044-check
  PRIVATE
    Result=$<C_COMPILER_ID:${lc_test}>
    Type_Is_${CMP0044_TYPE}
)
