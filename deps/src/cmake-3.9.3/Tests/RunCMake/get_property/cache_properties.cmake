function (check_cache_property var prop)
  get_property(gp_val
    CACHE "${var}"
    PROPERTY "${prop}")

  message("get_property: -->${gp_val}<--")
endfunction ()

set(var val CACHE STRING "doc")
set_property(CACHE var PROPERTY VALUE "") # empty
set_property(CACHE var PROPERTY ADVANCED TRUE)

check_cache_property(var VALUE)
check_cache_property(var ADVANCED)
check_cache_property(var noexist)
