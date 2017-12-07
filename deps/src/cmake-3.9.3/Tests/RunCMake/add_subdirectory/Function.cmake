function(my_add_subdirectory dir)
  set(var 2)
  message(STATUS "var='${var}' in my_add_subdirectory before add_subdirectory")
  add_subdirectory(${dir})
  message(STATUS "var='${var}' in my_add_subdirectory after add_subdirectory")
  message(STATUS "var_sub='${var_sub}' in my_add_subdirectory after add_subdirectory")
endfunction()

set(var 1)

message(STATUS "var='${var}' before my_add_subdirectory")
my_add_subdirectory(Function)
message(STATUS "var='${var}' after my_add_subdirectory")
message(STATUS "var_sub='${var_sub}' after my_add_subdirectory")

get_directory_property(sub_var DIRECTORY Function DEFINITION var)
message(STATUS "var='${sub_var}' taken from subdirectory")
