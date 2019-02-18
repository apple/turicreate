cmake_policy(SET CMP0070 NEW)
add_custom_target(foo)
file(GENERATE OUTPUT TARGET_EXISTS-generated.txt CONTENT "$<TARGET_EXISTS:foo>")
