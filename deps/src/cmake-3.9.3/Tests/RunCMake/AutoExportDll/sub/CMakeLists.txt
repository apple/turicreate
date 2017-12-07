add_library(autoexport2 SHARED sub.cxx)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Try msvc "big" object format.
  target_compile_options(autoexport2 PRIVATE /bigobj)
endif()
