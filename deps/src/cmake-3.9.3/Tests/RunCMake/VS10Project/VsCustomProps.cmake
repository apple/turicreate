enable_language(CXX)
add_library(foo foo.cpp)

set(props_file "${CMAKE_CURRENT_SOURCE_DIR}/my.props")

set_target_properties(foo PROPERTIES
    VS_USER_PROPS "${props_file}")
