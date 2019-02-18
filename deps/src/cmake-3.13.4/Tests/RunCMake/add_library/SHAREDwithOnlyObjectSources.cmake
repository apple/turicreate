enable_language(CXX)
add_library(ObjectLibDependency OBJECT test.cpp)

add_library(TestSharedLibWithoutSources SHARED)
target_sources(TestSharedLibWithoutSources PUBLIC $<TARGET_OBJECTS:ObjectLibDependency>)
