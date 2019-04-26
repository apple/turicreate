enable_language(CXX)
add_library(foo foo.cpp)

set_target_properties(foo PROPERTIES
    VS_DEBUGGER_ENVIRONMENT "my-debugger-environment $<TARGET_PROPERTY:foo,NAME>")
