enable_language(CXX)
add_library(ObjectLibDependency OBJECT test.cpp)

add_library(TestObjectLibWithoutSources OBJECT)
target_sources(TestObjectLibWithoutSources PUBLIC $<TARGET_OBJECTS:ObjectLibDependency>)
