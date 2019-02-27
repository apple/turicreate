configure_file(test.manifest.in test.manifest)
add_executable(MSManifest main.c ${CMAKE_CURRENT_BINARY_DIR}/test.manifest)

if(MSVC AND NOT MSVC_VERSION LESS 1400)
  add_custom_command(TARGET MSManifest POST_BUILD VERBATIM
    COMMAND ${CMAKE_COMMAND} -Dexe=$<TARGET_FILE:MSManifest>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check.cmake
    )
  add_executable(MSManifestNone main.c)
  set_property(TARGET MSManifestNone PROPERTY LINK_FLAGS "/MANIFEST:NO")
endif()
