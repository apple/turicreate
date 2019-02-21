enable_language(CXX)
add_library(ObjectLibDependency OBJECT test.cpp)

add_library(TestStaticLibWithoutSources STATIC)
target_sources(TestStaticLibWithoutSources PUBLIC $<TARGET_OBJECTS:ObjectLibDependency>)
