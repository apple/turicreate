set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/Scripts")

# Test include_guard with DIRECTORY scope

# Add subdirectory which includes DirScript three times:
# 1. Include at inner function scope
# 2. At directory scope
# 3. At another subdirectory to check that the guard is checked
# against parent directories
add_subdirectory(sub_dir_script1)
# Add another directory which includes DirScript
add_subdirectory(sub_dir_script2)

# check inclusions count
get_property(dir_count GLOBAL PROPERTY DIR_SCRIPT_COUNT)
if(NOT dir_count EQUAL 2)
  message(FATAL_ERROR
          "Wrong DIR_SCRIPT_COUNT value: ${dir_count}, expected: 2")
endif()
