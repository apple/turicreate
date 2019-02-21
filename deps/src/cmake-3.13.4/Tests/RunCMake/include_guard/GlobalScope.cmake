set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/Scripts")

# Test GLOBAL include guard
add_subdirectory(global_script_dir)
include(GlobScript)

get_property(glob_count GLOBAL PROPERTY GLOB_SCRIPT_COUNT)
if(NOT glob_count EQUAL 1)
  message(FATAL_ERROR
          "Wrong GLOB_SCRIPT_COUNT value: ${glob_count}, expected: 1")
endif()
