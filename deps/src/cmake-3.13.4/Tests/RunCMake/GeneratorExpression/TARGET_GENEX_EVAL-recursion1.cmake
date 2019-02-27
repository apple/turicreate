
enable_language(C)

add_library (recursion SHARED empty.c)
set_property (TARGET recursion PROPERTY CUSTOM_PROPERTY "$<TARGET_GENEX_EVAL:recursion,$<TARGET_PROPERTY:CUSTOM_PROPERTY>>")

add_custom_target (drive
  COMMAND echo "$<TARGET_GENEX_EVAL:recursion,$<TARGET_PROPERTY:recursion,CUSTOM_PROPERTY>>"
  DEPENDS recursion)
