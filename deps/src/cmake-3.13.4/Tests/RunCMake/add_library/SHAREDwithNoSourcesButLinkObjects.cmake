enable_language(CXX)
add_library(ObjectLibDependency OBJECT test.cpp)

add_library(TestSharedLibWithoutSources SHARED)
target_link_libraries(TestSharedLibWithoutSources PUBLIC $<TARGET_OBJECTS:ObjectLibDependency>)
