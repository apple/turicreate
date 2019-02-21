enable_language(CXX)
add_library(ObjectLibDependency OBJECT test.cpp)

add_library(TestObjectLibWithoutSources OBJECT)
target_link_libraries(TestObjectLibWithoutSources PUBLIC $<TARGET_OBJECTS:ObjectLibDependency>)
