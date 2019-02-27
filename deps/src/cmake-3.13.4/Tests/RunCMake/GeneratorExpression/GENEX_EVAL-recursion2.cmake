
enable_language(C)

add_library(recursion SHARED empty.c)
set_property (TARGET recursion PROPERTY CUSTOM_PROPERTY1 "$<GENEX_EVAL:$<TARGET_PROPERTY:CUSTOM_PROPERTY2>>")
set_property (TARGET recursion PROPERTY CUSTOM_PROPERTY2 "$<GENEX_EVAL:$<TARGET_PROPERTY:CUSTOM_PROPERTY1>>")

add_custom_target (drive
  COMMAND echo "$<TARGET_GENEX_EVAL:recursion,$<TARGET_PROPERTY:recursion,CUSTOM_PROPERTY1>>"
  DEPENDS recursion)
