
enable_language(C)

add_library (recursion1 SHARED empty.c)
set_property (TARGET recursion1 PROPERTY CUSTOM_PROPERTY1 "$<TARGET_GENEX_EVAL:recursion2,$<TARGET_PROPERTY:recursion2,CUSTOM_PROPERTY2>>")

add_library (recursion2 SHARED empty.c)
set_property (TARGET recursion2 PROPERTY CUSTOM_PROPERTY2 "$<TARGET_GENEX_EVAL:recursion1,$<TARGET_PROPERTY:recursion1,CUSTOM_PROPERTY1>>")

add_custom_target (drive
  COMMAND echo "$<TARGET_GENEX_EVAL:recursion1,$<TARGET_PROPERTY:recursion1,CUSTOM_PROPERTY1>>"
  DEPENDS recursion)
