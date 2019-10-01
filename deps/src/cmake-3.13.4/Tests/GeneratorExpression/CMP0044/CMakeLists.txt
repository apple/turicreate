
string(TOLOWER ${CMAKE_C_COMPILER_ID} lc_test)
if (lc_test STREQUAL CMAKE_C_COMPILER_ID)
  string(TOUPPER ${CMAKE_C_COMPILER_ID} lc_test)
  if (lc_test STREQUAL CMAKE_C_COMPILER_ID)
    message(SEND_ERROR "Try harder.")
  endif()
endif()

if (CMP0044_TYPE)
  cmake_policy(SET CMP0044 ${CMP0044_TYPE})
endif()

add_library(cmp0044-check-${CMP0044_TYPE} cmp0044-check.cpp)
target_compile_definitions(cmp0044-check-${CMP0044_TYPE}
  PRIVATE
    Result=$<C_COMPILER_ID:${lc_test}>
    Type_Is_${CMP0044_TYPE}
)
