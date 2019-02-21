# Needed for source property tests
enable_language(C)

#=================================================
# Directory property chaining
#=================================================

foreach(i RANGE 1 5)
  foreach(propType DIRECTORY TARGET SOURCE TEST)
    define_property(${propType} PROPERTY USER_PROP${i} INHERITED
      BRIEF_DOCS "Brief" FULL_DOCS "Full"
    )
  endforeach()
endforeach()

get_property(val DIRECTORY PROPERTY USER_PROP1)
message(STATUS "TopDir-to-nothing chaining: '${val}'")

set_property(GLOBAL    PROPERTY USER_PROP1 vGlobal)
set_property(GLOBAL    PROPERTY USER_PROP2 vGlobal)
set_property(DIRECTORY PROPERTY USER_PROP2 vTopDir)
set_property(GLOBAL    PROPERTY USER_PROP3 vGlobal)
set_property(DIRECTORY PROPERTY USER_PROP4 vTopDir)

get_property(val DIRECTORY PROPERTY USER_PROP1)
message(STATUS "TopDir-to-global chaining: '${val}'")

get_property(val DIRECTORY PROPERTY USER_PROP2)
message(STATUS "TopDir no chaining required: '${val}'")

set_property(DIRECTORY APPEND PROPERTY USER_PROP3 aTopDir)
get_property(val DIRECTORY PROPERTY USER_PROP3)
message(STATUS "TopDir unset append chaining: '${val}'")

set_property(DIRECTORY APPEND PROPERTY USER_PROP4 aTopDir)
get_property(val DIRECTORY PROPERTY USER_PROP4)
message(STATUS "TopDir preset append chaining: '${val}'")

add_subdirectory(USER_PROP_INHERITED)

#=================================================
# The other property types all chain the same way
#=================================================
macro(__chainToDirTests propType)
  string(TOUPPER ${propType} propTypeUpper)

  get_property(val ${propTypeUpper} ${propType}1 PROPERTY USER_PROP2)
  message(STATUS "${propType}-to-directory chaining: '${val}'")

  set_property(${propTypeUpper} ${propType}1 APPEND PROPERTY USER_PROP2 a${propType})
  get_property(val ${propTypeUpper} ${propType}1 PROPERTY USER_PROP2)
  message(STATUS "${propType} unset append chaining: '${val}'")

  set_property(${propTypeUpper} ${propType}1 PROPERTY USER_PROP1 v${propType})
  get_property(val ${propTypeUpper} ${propType}1 PROPERTY USER_PROP1)
  message(STATUS "${propType} no chaining required: '${val}'")

  set_property(${propTypeUpper} ${propType}1 APPEND PROPERTY USER_PROP1 a${propType})
  get_property(val ${propTypeUpper} ${propType}1 PROPERTY USER_PROP1)
  message(STATUS "${propType} preset append chaining: '${val}'")

  get_property(val ${propTypeUpper} ${propType}2 PROPERTY USER_PROP5)
  message(STATUS "${propType} undefined get chaining: '${val}'")

  set_property(${propTypeUpper} ${propType}2 APPEND PROPERTY USER_PROP5 a${propType})
  get_property(val ${propTypeUpper} ${propType}2 PROPERTY USER_PROP5)
  message(STATUS "${propType} undefined append chaining: '${val}'")
endmacro()

add_custom_target(Target1)
add_custom_target(Target2)
__chainToDirTests(Target)

foreach(i RANGE 1 2)
  set(Source${i} "${CMAKE_CURRENT_BINARY_DIR}/src${i}.c")
  file(WRITE ${Source${i}} "int foo${i}() { return ${i}; }")
endforeach()
add_library(srcProps OBJECT ${Source1} ${Source2})
__chainToDirTests(Source)

add_test(NAME Test1 COMMAND ${CMAKE_COMMAND} -E touch_nocreate iDoNotExist)
add_test(NAME Test2 COMMAND ${CMAKE_COMMAND} -E touch_nocreate iDoNotExist)
__chainToDirTests(Test)
