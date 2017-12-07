project(unity)

# We depend on the open source src/unity/python/turicreate
# to fill
#  - INSTALLATION_BINARY_FILES
#  - INSTALLATION_EXTENSIONS
#  - INSTALLATION_BINARY_TARGETS
#
# These can be amended locally to set up alternative behavior

set(INSTALLATION_BINARY_TARGETS
  unity
  minipsutil
)
set(INSTALLATION_BINARY_FILES)
set(INSTALLATION_EXTENSIONS)
# add the propretary extensions
get_property(EXTENSIONS_LIST DIRECTORY "../../extensions" PROPERTY EXTENSIONS_LIST)
foreach(loop_var ${EXTENSIONS_LIST})
  message(STATUS "Copying Extension to turicreate python directory ${loop_var}")
  list(APPEND INSTALLATION_EXTENSIONS ${loop_var})
endforeach()

make_copy_target(release_binaries ALL
        TARGETS
        ${UNITY_ALL_LIBRARIES}
        ${INSTALLATION_BINARY_TARGETS}
        ${INSTALLATION_EXTENSIONS}
        FILES
        ${INSTALLATION_BINARY_FILES}
        ${INSTALLATION_SYSTEM_BINARY_FILES}
        )

ADD_CUSTOM_TARGET (clean_turicreate_binaries)
ADD_CUSTOM_COMMAND(
  COMMENT "clean turicreate binaries"
  COMMAND rm -f ${INSTALLATION_BINARY_TARGETS} ${UNITY_ALL_LIBRARIES} *.so *.dylib *.dll *.jar
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  TARGET clean_turicreate_binaries
)

add_subdirectory(cython)

set_property(DIRECTORY PROPERTY INSTALLATION_EXTENSIONS "${INSTALLATION_EXTENSIONS}")
set_property(DIRECTORY PROPERTY INSTALLATION_BINARY_TARGETS "${INSTALLATION_BINARY_TARGETS}")
set_property(DIRECTORY PROPERTY INSTALLATION_BINARY_FILES "${INSTALLATION_BINARY_FILES}")

if (APPLE)
  add_custom_target(
    visualization_client_app ALL
    DEPENDS visualization_client
    COMMAND cp -a "${CMAKE_BINARY_DIR}/src/visualization/${CMAKE_BUILD_TYPE}/Turi Create Visualization.app" ${CMAKE_CURRENT_BINARY_DIR}
  )
endif()

