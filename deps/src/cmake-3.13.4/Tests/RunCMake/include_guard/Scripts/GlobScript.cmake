include_guard(GLOBAL)

set(prop_name GLOB_SCRIPT_COUNT)
get_property(count_is_set GLOBAL PROPERTY ${prop_name} SET)

if(NOT count_is_set)
  set_property(GLOBAL PROPERTY ${prop_name} 1)
else()
  get_property(count GLOBAL PROPERTY ${prop_name})
  math(EXPR count "${count} + 1")
  set_property(GLOBAL PROPERTY ${prop_name} ${count})
endif()
