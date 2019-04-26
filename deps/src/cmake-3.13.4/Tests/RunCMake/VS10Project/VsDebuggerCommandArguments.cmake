enable_language(CXX)
add_library(foo foo.cpp)

set_target_properties(foo PROPERTIES
    VS_DEBUGGER_COMMAND_ARGUMENTS "my-debugger-command-arguments $<TARGET_PROPERTY:foo,NAME>")
