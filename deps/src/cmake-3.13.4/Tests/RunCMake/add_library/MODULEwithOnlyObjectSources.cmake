enable_language(CXX)
add_library(ObjectLibDependency OBJECT test.cpp)

add_library(TestModuleLibWithoutSources MODULE)
target_sources(TestModuleLibWithoutSources PUBLIC $<TARGET_OBJECTS:ObjectLibDependency>)
