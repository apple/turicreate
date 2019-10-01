cmake_policy(SET CMP0046 NEW)

add_custom_target(foo)
add_dependencies(foo bar)
