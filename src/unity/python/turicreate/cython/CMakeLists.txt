project(unity_cython)

set(CMAKE_C_FLAGS_DEBUG
  "${EXTERNAL_CMAKE_C_FLAGS_DEBUG}")

set(CMAKE_CXX_FLAGS_DEBUG
  "${EXTERNAL_CMAKE_CXX_FLAGS_DEBUG}")

include_directories(${NUMPY_INCLUDE_DIRS})

function(cython_add_turicreate_module _module_name _sources)
  set_source_files_properties( ${_sources} PROPERTIES CYTHON_IS_CXX TRUE)
  cython_add_module( ${_module_name} ${_sources} )
  set_property(TARGET ${_module_name} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-rpath,\\$ORIGIN -Wl,-rpath,\\$ORIGIN/.. -Wl,-rpath,${CMAKE_CURRENT_BINARY_DIR} ")
  target_link_libraries(${_module_name} boost nanomsg unity_shared)
  set_property(TARGET ${_module_name} APPEND_STRING PROPERTY COMPILE_FLAGS " -include ${CMAKE_CURRENT_SOURCE_DIR}/cython_cpp_error_handler.hpp")
  set_property(TARGET ${_module_name} APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-writable-strings -Wno-sign-compare -Wno-parentheses-equality -Wno-error")

  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set_property(TARGET ${_module_name} APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-sign-compare -Wno-sign-compare -Wno-parentheses-equality -Wno-error")
  endif()

  add_dependencies(${_module_name} release_binaries)
endfunction()


add_custom_target(cython_targets ALL)
file(GLOB cython_sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.pyx")
foreach(source ${cython_sources})
  set(target "")
  string(REPLACE ".pyx" "" target ${source})
  cython_add_turicreate_module(${target} ${source})
  set_target_properties(${target} PROPERTIES OUTPUT_NAME ${target})
  add_dependencies(cython_targets ${target})
endforeach()


ADD_CUSTOM_TARGET (clean_cython)
ADD_CUSTOM_COMMAND(
  COMMENT "clean cython"
  COMMAND
  rm -f *.so *.dbg *.a *.o *.dll *.pyd *.obj
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  TARGET clean_cython
)
if(WIN32)
  make_copy_target(cython_deps ALL
    TARGETS
    ${INSTALLATION_BINARY_TARGETS}
    FILES
    ${INSTALLATION_BINARY_FILES}
    ${INSTALLATION_SYSTEM_BINARY_FILES}
    )
endif()
