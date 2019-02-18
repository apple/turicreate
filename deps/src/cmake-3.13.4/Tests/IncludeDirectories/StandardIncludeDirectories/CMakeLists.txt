# Normally this variable should be set by a platform information module or
# a toolchain file, but for purposes of this test we simply set it here.
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/StdDir)

add_executable(StandardIncludeDirectories main.c)
