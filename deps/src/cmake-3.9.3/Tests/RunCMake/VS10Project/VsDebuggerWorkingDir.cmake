enable_language(CXX)
add_library(foo foo.cpp)

set_target_properties(foo PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "my-debugger-directory")
