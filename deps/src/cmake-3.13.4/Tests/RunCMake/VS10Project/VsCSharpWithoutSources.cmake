enable_language(CSharp)

add_library(foo SHARED
  "${CMAKE_CURRENT_LIST_FILE}")

set_target_properties(foo PROPERTIES
  LINKER_LANGUAGE CSharp)
