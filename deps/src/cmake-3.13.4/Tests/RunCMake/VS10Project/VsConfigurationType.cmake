enable_language(CXX)
add_library(foo foo.cpp)
set_target_properties(foo PROPERTIES VS_CONFIGURATION_TYPE "MyValue")
