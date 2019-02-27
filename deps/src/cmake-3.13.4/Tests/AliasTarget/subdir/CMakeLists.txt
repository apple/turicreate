
add_library(tgt STATIC empty.cpp)
add_library(Sub::tgt ALIAS tgt)

# foo comes from the top-level CMakeLists.txt
add_library(Top::foo ALIAS foo)
get_target_property(some_prop Top::foo SOME_PROP)
target_link_libraries(tgt Top::foo)
