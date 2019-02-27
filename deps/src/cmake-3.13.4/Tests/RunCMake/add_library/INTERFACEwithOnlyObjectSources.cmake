enable_language(CXX)
add_library(ObjectLibDependency OBJECT test.cpp)

add_library(TestInterfaceLibWithoutSources INTERFACE)
target_sources(TestInterfaceLibWithoutSources INTERFACE $<TARGET_OBJECTS:ObjectLibDependency>)
