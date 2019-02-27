project(ObjectLibraryB C)

# Add -fPIC so objects can be used in shared libraries.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(B OBJECT b1.c b2.c)
target_include_directories(B PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(B PUBLIC B_DEF)

add_library(Bexport OBJECT b1.c b2.c)
set_property(TARGET Bexport PROPERTY COMPILE_DEFINITIONS Bexport)
target_include_directories(Bexport PRIVATE $<TARGET_PROPERTY:B,INTERFACE_INCLUDE_DIRECTORIES>)
target_compile_definitions(Bexport PRIVATE $<TARGET_PROPERTY:B,INTERFACE_COMPILE_DEFINITIONS>)
