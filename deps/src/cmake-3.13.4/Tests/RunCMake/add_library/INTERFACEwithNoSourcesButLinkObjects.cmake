enable_language(CXX)
add_library(ObjectLibDependency OBJECT test.cpp)

add_library(TestInterfaceLibWithoutSources INTERFACE)
target_link_libraries(TestInterfaceLibWithoutSources INTERFACE $<TARGET_OBJECTS:ObjectLibDependency>)
