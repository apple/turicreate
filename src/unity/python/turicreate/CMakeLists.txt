project(unity)

if(UNIX AND NOT APPLE)
  set(LINUX TRUE)
endif()

# add the propretary extensions
get_property(EXTENSIONS_LIST DIRECTORY "../../extensions" PROPERTY EXTENSIONS_LIST)
foreach(loop_var ${EXTENSIONS_LIST})
  message(STATUS "Building extension shared library in turicreate python directory: ${loop_var}")
  make_shared_library_from_static(${loop_var}
    REQUIRES ${loop_var}_static
    OUTPUT_NAME ${loop_var}
  )
endforeach()

if(APPLE AND HAS_MLCUSTOMMODEL)
  set(EXTRA_REQUIRES objcapi)
endif()

make_shared_library_from_static(unity_shared
      REQUIRES unity TuriCreate_static ${EXTRA_REQUIRES}
      OUTPUT_NAME unity_shared
      EXPORT_MAP ${CMAKE_SOURCE_DIR}/cmake/stdcxx_reduce_exports.ver)

make_symlink_target(release_binaries
        FILES
          ${INSTALLATION_SYSTEM_BINARY_FILES}
        )

ADD_CUSTOM_TARGET (clean_turicreate_binaries)
ADD_CUSTOM_COMMAND(
  COMMENT "clean turicreate binaries"
  COMMAND rm -f *.so *.so.* *.dylib *.dll *.jar
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  TARGET clean_turicreate_binaries
)

add_subdirectory(cython)

if (APPLE)
  make_symlink_target(
    visualization_client_app
    FILES
    "${CMAKE_BINARY_DIR}/src/visualization/${CMAKE_BUILD_TYPE}/Turi Create Visualization.app"
  )
  add_dependencies(visualization_client_app visualization_client)
elseif(LINUX)
  add_custom_target(
    visualization_client_app ALL
    DEPENDS visualization_client
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/Turi Create Visualization"
    COMMAND cp -a "${CMAKE_BINARY_DIR}/src/visualization/TcViz/*" "${CMAKE_CURRENT_BINARY_DIR}/Turi Create Visualization"
  )
endif()