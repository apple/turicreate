enable_language(CXX)
add_library(ObjectLibDependency OBJECT test.cpp)

add_library(TestStaticLibWithoutSources STATIC)
target_link_libraries(TestStaticLibWithoutSources PUBLIC $<TARGET_OBJECTS:ObjectLibDependency>)
