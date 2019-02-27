set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/Scripts")

# Test include_guard with VARIABLE scope
function(var_include_func)
  # Include twice in the same scope
  include(VarScript)
  include(VarScript)
  get_property(var_count GLOBAL PROPERTY VAR_SCRIPT_COUNT)
  if(NOT var_count EQUAL 1)
    message(FATAL_ERROR
            "Wrong VAR_SCRIPT_COUNT value: ${var_count}, expected: 1")
  endif()
endfunction()

var_include_func()

# Check again that include_guard has been reset
include(VarScript)

get_property(var_count GLOBAL PROPERTY VAR_SCRIPT_COUNT)
if(NOT var_count EQUAL 2)
  message(FATAL_ERROR
          "Wrong VAR_SCRIPT_COUNT value: ${var_count}, expected: 2")
endif()
