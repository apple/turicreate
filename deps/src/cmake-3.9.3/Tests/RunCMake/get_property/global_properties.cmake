function (check_global_property prop)
  get_cmake_property(gcp_val "${prop}")
  get_property(gp_val
    GLOBAL
    PROPERTY "${prop}")

  message("get_cmake_property: -->${gcp_val}<--")
  message("get_property: -->${gp_val}<--")
endfunction ()

set_property(GLOBAL PROPERTY empty "")
set_property(GLOBAL PROPERTY custom value)

check_global_property(empty)
check_global_property(custom)
check_global_property(noexist)
