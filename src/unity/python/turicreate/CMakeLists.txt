project(unity)

# We depend on the open source src/unity/python/turicreate
# to fill
#  - INSTALLATION_BINARY_FILES
#  - INSTALLATION_EXTENSIONS
#
# These can be amended locally to set up alternative behavior

if(UNIX AND NOT APPLE)
  set(LINUX TRUE)
endif()

set(INSTALLATION_BINARY_FILES)
set(INSTALLATION_EXTENSIONS)
# add the propretary extensions
get_property(EXTENSIONS_LIST DIRECTORY "../../extensions" PROPERTY EXTENSIONS_LIST)
foreach(loop_var ${EXTENSIONS_LIST})
  message(STATUS "Copying Extension to turicreate python directory ${loop_var}")
  list(APPEND INSTALLATION_EXTENSIONS ${loop_var})
endforeach()

make_library(unity_shared
  SOURCES
    ${unity_shared_deps}
    ${_TC_DEFAULT_SERVER_INITIALIZER}
    ${_TC_COMMON_OBJECTS}
  REQUIRES
    ${_TC_COMMON_REQUIREMENTS}
  SHARED
  SHARED_ALL_DEFINED
  DEAD_STRIP
)

make_copy_target(release_binaries
        TARGETS
        ${INSTALLATION_EXTENSIONS}
        FILES
        ${INSTALLATION_BINARY_FILES}
        ${INSTALLATION_SYSTEM_BINARY_FILES}
        )

ADD_CUSTOM_TARGET (clean_turicreate_binaries)
ADD_CUSTOM_COMMAND(
  COMMENT "clean turicreate binaries"
  COMMAND rm -f *.so *.dylib *.dll *.jar
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  TARGET clean_turicreate_binaries
)

add_subdirectory(_cython)

set_property(DIRECTORY PROPERTY INSTALLATION_EXTENSIONS "${INSTALLATION_EXTENSIONS}")
set_property(DIRECTORY PROPERTY INSTALLATION_BINARY_FILES "${INSTALLATION_BINARY_FILES}")

if (APPLE)
  make_copy_target(
    visualization_client_app
    FILES
    ${CMAKE_SOURCE_DIR}/src/unity/python/turicreate/visualization/vega_3.2.1.js
    ${CMAKE_SOURCE_DIR}/src/unity/python/turicreate/visualization/vg2png
    ${CMAKE_SOURCE_DIR}/src/unity/python/turicreate/visualization/vg2svg
    DIRECTORIES
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
  add_custom_target(
    visualization_vega ALL
    DEPENDS visualization_client
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/src/unity/python/turicreate/visualization/vega_3.2.1.js" "${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/src/unity/python/turicreate/visualization/vg2png" "${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/src/unity/python/turicreate/visualization/vg2svg" "${CMAKE_CURRENT_BINARY_DIR}"
  )
endif()
