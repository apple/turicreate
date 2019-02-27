enable_language(C)
add_executable(some main.c)
add_executable(another main.c)
set_property(TARGET another PROPERTY XCODE_ATTRIBUTE_TEST_HOST "$<NOTAGENEX>")
