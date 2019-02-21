cmake_minimum_required(VERSION 3.12)
project(ImportedSameName C)

add_subdirectory(A)
add_subdirectory(B)

add_executable(ImportedSameName main.c)
target_link_libraries(ImportedSameName PRIVATE ifaceA ifaceB)
