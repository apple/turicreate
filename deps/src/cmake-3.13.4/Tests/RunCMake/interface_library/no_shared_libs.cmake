
cmake_minimum_required(VERSION 2.8.12.20131009)
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS FALSE)
add_library(foo INTERFACE)
target_compile_definitions(foo INTERFACE FOO_DEFINE)
