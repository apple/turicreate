enable_language(C)
add_executable(some main.c)
add_executable(another main.c)
set_target_properties(another PROPERTIES
  # per target attribute with genex
  XCODE_ATTRIBUTE_TEST_HOST "$<TARGET_FILE:some>"
  # per target attribute with variant
  XCODE_ATTRIBUTE_CONFIG_SPECIFIC[variant=Release] "release"
  XCODE_ATTRIBUTE_CONFIG_SPECIFIC "general")

# global attribute with genex
set(CMAKE_XCODE_ATTRIBUTE_ANOTHER_GLOBAL "$<TARGET_FILE:another>")

# global attribute with variant
set(CMAKE_XCODE_ATTRIBUTE_ANOTHER_CONFIG "general")
set(CMAKE_XCODE_ATTRIBUTE_ANOTHER_CONFIG[variant=Debug] "debug")
