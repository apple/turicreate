project(Turi)


if(UNIX AND NOT APPLE)
  set(LINUX TRUE)
endif()

if(${TC_BUILD_VISUALIZATION_CLIENT})

  find_program(NPM npm)
  if(NOT NPM)
    message(FATAL_ERROR "npm not found. Node.js 6.x or later with npm is required to build Turi Create.")
  endif()

  if(APPLE)
    add_custom_target(visualization_client
      COMMAND xcodebuild -project "${CMAKE_CURRENT_SOURCE_DIR}/Turi Create Visualization.xcodeproj/" -configuration ${CMAKE_BUILD_TYPE} SYMROOT=${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Building visualization client via xcodebuild"
    )
  elseif(LINUX)
    include(ExternalProject)

    ExternalProject_Add(
        cef
        PREFIX "cef"
        URL http://opensource.spotify.com/cefbuilds/cef_binary_3.3282.1731.gfc9a4fa_linux64_minimal.tar.bz2
        URL_HASH SHA1=4eeeabee642bfe68f561566e0f249f86afaf6634
        DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/cef/src/cef
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ${CMAKE_COMMAND} .
        BUILD_COMMAND "make"
        INSTALL_COMMAND ""
    )


    set(CEF_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/deps/cef/src/cef/Resources)

    set(TC_VIZ_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/TcViz")
    set(TC_VIZ_HTML_DIRECTORY "${TC_VIZ_DIRECTORY}/html")

    set(CEF_RESOURCES_LOCALE "${CMAKE_CURRENT_BINARY_DIR}/deps/cef/src/cef/Resources/locales")
    set(TC_VIZ_LOCALES_DIRECTORY "${TC_VIZ_DIRECTORY}/locales")

    set(TCVIZ_SRCS
      src/layer.cpp
      src/handler.cpp
      src/javascript_caller.cpp
      src/pipe.cpp
      src/main.cpp
      src/handler_linux.cpp
    )

    set(LIB_CEF_DLL_WRAPPER "${CMAKE_CURRENT_BINARY_DIR}/deps/cef/src/cef/libcef_dll_wrapper/libcef_dll_wrapper.a")
    set(LIB_CEF "${CMAKE_CURRENT_BINARY_DIR}/deps/cef/src/cef/Release/libcef.so")
    set(LIB_CEF_SO_DEPS
      ${CMAKE_CURRENT_BINARY_DIR}/deps/cef/src/cef/Release/libEGL.so
      ${CMAKE_CURRENT_BINARY_DIR}/deps/cef/src/cef/Release/libGLESv2.so)
    set(LIB_CEF_SWIFTSHADER_SO_DEPS
      ${CMAKE_CURRENT_BINARY_DIR}/deps/cef/src/cef/Release/swiftshader/libEGL.so
      ${CMAKE_CURRENT_BINARY_DIR}/deps/cef/src/cef/Release/swiftshader/libGLESv2.so)
    set(V8_CONTEXT_SNAPSHOT "${CMAKE_CURRENT_BINARY_DIR}/deps/cef/src/cef/Release/v8_context_snapshot.bin")
    set(SNAPSHOT_BLOB "${CMAKE_CURRENT_BINARY_DIR}/deps/cef/src/cef/Release/snapshot_blob.bin")
    set(NATIVES_BLOB "${CMAKE_CURRENT_BINARY_DIR}/deps/cef/src/cef/Release/natives_blob.bin")
    set(TC_NPM_BUILD "${CMAKE_CURRENT_SOURCE_DIR}/Turi Create Visualization/src/user_interface/build")

    include_directories(
      ${CMAKE_CURRENT_BINARY_DIR}/deps/cef/src/cef
    )

    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${TC_VIZ_DIRECTORY}")

    add_executable(visualization_client "${TCVIZ_SRCS}")

    set_target_properties(visualization_client
      PROPERTIES LINK_FLAGS "-Wl,--unresolved-symbols=ignore-in-shared-libs")

    target_link_libraries(visualization_client
      ${LIB_CEF_DLL_WRAPPER}
      ${LIB_CEF}
      X11
      pthread
    )

    add_dependencies(visualization_client cef)

    add_custom_command(
            TARGET visualization_client PRE_LINK

            COMMAND npm install

            COMMAND npm run build

            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Turi Create Visualization/src/user_interface"

            COMMENT "Building NPM Package for Turi Create Visualization"

            VERBATIM
    )

    add_custom_command(
            TARGET visualization_client POST_BUILD

            COMMAND ${CMAKE_COMMAND} -E make_directory
                    ${TC_VIZ_DIRECTORY}

            COMMAND ${CMAKE_COMMAND} -E make_directory
                    ${TC_VIZ_HTML_DIRECTORY}

            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${TC_NPM_BUILD}
                    ${TC_VIZ_HTML_DIRECTORY}

            COMMAND ${CMAKE_COMMAND} -E make_directory
                    ${TC_VIZ_LOCALES_DIRECTORY}

            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${CEF_RESOURCES}
                    ${TC_VIZ_DIRECTORY}

            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${CEF_RESOURCES_LOCALE}
                    ${TC_VIZ_LOCALES_DIRECTORY}

            COMMAND ${CMAKE_COMMAND} -E copy
                    ${LIB_CEF}
                    ${TC_VIZ_DIRECTORY}

            COMMAND ${CMAKE_COMMAND} -E copy
                    ${V8_CONTEXT_SNAPSHOT}
                    ${TC_VIZ_DIRECTORY}

            COMMAND ${CMAKE_COMMAND} -E copy
                    ${V8_CONTEXT_SNAPSHOT}
                    ${TC_VIZ_DIRECTORY}

            COMMAND ${CMAKE_COMMAND} -E copy
                    ${SNAPSHOT_BLOB}
                    ${TC_VIZ_DIRECTORY}

            COMMAND ${CMAKE_COMMAND} -E copy
                    ${NATIVES_BLOB}
                    ${TC_VIZ_DIRECTORY}

            COMMAND ${CMAKE_COMMAND} -E make_directory
                    ${TC_VIZ_DIRECTORY}/swiftshader

            COMMAND ${CMAKE_COMMAND} -E copy
                    ${LIB_CEF_SO_DEPS}
                    ${TC_VIZ_DIRECTORY}

            COMMAND ${CMAKE_COMMAND} -E copy
                    ${LIB_CEF_SWIFTSHADER_SO_DEPS}
                    ${TC_VIZ_DIRECTORY}/swiftshader

            VERBATIM
    )
  else()
    make_empty_library(visualization_client)
  endif()
else()
  make_empty_library(visualization_client)
endif()
