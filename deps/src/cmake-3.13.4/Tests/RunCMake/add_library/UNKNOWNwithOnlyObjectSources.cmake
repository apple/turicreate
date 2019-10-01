enable_language(CXX)
add_library(ObjectLibDependency OBJECT test.cpp)

add_library(TestUnknownLibWithoutSources UNKNOWN IMPORTED)
target_sources(TestUnknownLibWithoutSources PUBLIC $<TARGET_OBJECTS:ObjectLibDependency>)
