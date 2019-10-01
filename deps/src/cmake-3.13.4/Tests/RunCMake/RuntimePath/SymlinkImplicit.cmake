enable_language(C)

set(lib_dir ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(lib_link ${CMAKE_CURRENT_BINARY_DIR}/libLink)
set(lib_always ${CMAKE_CURRENT_BINARY_DIR}/libAlways)
file(MAKE_DIRECTORY ${lib_dir})
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink lib ${lib_link})
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink lib ${lib_always})

add_library(A SHARED A.c)
list(APPEND CMAKE_C_IMPLICIT_LINK_DIRECTORIES ${lib_dir})
set_property(TARGET A PROPERTY LIBRARY_OUTPUT_DIRECTORY ${lib_link})

add_executable(exe main.c)
target_link_libraries(exe A)
set_property(TARGET exe PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set_property(TARGET exe PROPERTY BUILD_RPATH ${lib_always})
