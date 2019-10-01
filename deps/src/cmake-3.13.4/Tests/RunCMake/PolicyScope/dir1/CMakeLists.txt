

add_library(foo STATIC foo.cpp)
string(TOLOWER ${CMAKE_CXX_COMPILER_ID} compiler_id)
target_compile_definitions(foo PRIVATE Foo=$<CXX_COMPILER_ID:${compiler_id}>)
