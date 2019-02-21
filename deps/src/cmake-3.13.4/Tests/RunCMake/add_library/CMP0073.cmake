enable_language(C)

add_library(warn empty.c)
target_link_libraries(warn bar)
message(STATUS "warn_LIB_DEPENDS='${warn_LIB_DEPENDS}'")

cmake_policy(SET CMP0073 OLD)
add_library(old empty.c)
target_link_libraries(old bar)
message(STATUS "old_LIB_DEPENDS='${old_LIB_DEPENDS}'")

cmake_policy(SET CMP0073 NEW)
add_library(new empty.c)
target_link_libraries(new bar)
message(STATUS "new_LIB_DEPENDS='${new_LIB_DEPENDS}'")
if(DEFINED new_LIB_DEPENDS)
  message(FATAL_ERROR "new_LIB_DEPENDS set but should not be")
endif()
