enable_language(CXX)
add_library(ObjectLibDependency OBJECT test.cpp)

add_library(TestUnknownLibWithoutSources UNKNOWN IMPORTED)
target_link_libraries(TestUnknownLibWithoutSources INTERFACE $<TARGET_OBJECTS:ObjectLibDependency>)
