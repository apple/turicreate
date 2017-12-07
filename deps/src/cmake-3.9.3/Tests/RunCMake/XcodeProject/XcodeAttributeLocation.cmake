enable_language(C)
add_executable(some main.c)
set_property(TARGET some PROPERTY XCODE_ATTRIBUTE_DEPLOYMENT_LOCATION YES)
