cmake_policy(SET CMP0070 NEW)
add_custom_target(foo)
file(GENERATE OUTPUT TARGET_NAME_IF_EXISTS-generated.txt CONTENT "$<TARGET_NAME_IF_EXISTS:foo>")
