cmake_policy(SET CMP0046 OLD)

add_custom_target(foo)
add_dependencies(foo bar)
