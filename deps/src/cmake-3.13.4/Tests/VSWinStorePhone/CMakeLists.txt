cmake_minimum_required(VERSION 3.0)
project(VSWinStorePhone)
if(MSVC_VERSION GREATER 1899)
  set(COMPILER_VERSION "14")
elseif(MSVC_VERSION GREATER 1700)
  set(COMPILER_VERSION "12")
elseif(MSVC_VERSION GREATER 1600)
  set(COMPILER_VERSION "11")
endif()

set (APP_MANIFEST_NAME Package.appxmanifest)
if("${CMAKE_SYSTEM_NAME}" STREQUAL "WindowsPhone")
  set(PLATFORM WP)
  add_definitions("-DPHONE")
  if("${CMAKE_SYSTEM_VERSION}" STREQUAL "8.0")
    set(APP_MANIFEST_NAME WMAppManifest.xml)
    set(WINDOWS_PHONE8 1)
  endif()
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "WindowsStore")
  set(PLATFORM STORE)
else()
  set(PLATFORM DESKTOP)
  message(FATAL_ERROR "This app supports Store / Phone only. Please edit the target platform.")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(EXE_NAME Direct3DApp1)
set(SHORT_NAME ${EXE_NAME})
set(PACKAGE_GUID "6514377e-dfd4-4cdb-80df-4e0366346efc")

if (NOT "${PLATFORM}" STREQUAL "DESKTOP")
  configure_file(
    cmake/Package_vc${COMPILER_VERSION}.${PLATFORM}.appxmanifest.in
    ${CMAKE_CURRENT_BINARY_DIR}/${APP_MANIFEST_NAME}
    @ONLY)
endif()

set(SOURCE_FILES
  Direct3DApp1/CubeRenderer.cpp
  Direct3DApp1/Direct3DApp1.cpp
  Direct3DApp1/Direct3DBase.cpp
  Direct3DApp1/pch.cpp
  )

set(HEADER_FILES
  Direct3DApp1/BasicTimer.h
  Direct3DApp1/CubeRenderer.h
  Direct3DApp1/Direct3DApp1.h
  Direct3DApp1/Direct3DBase.h
  Direct3DApp1/DirectXHelper.h
  Direct3DApp1/pch.h
  )

set(PIXELSHADER_FILES
  Direct3DApp1/SimplePixelShader.hlsl
  )

set(VERTEXSHADER_FILES
  Direct3DApp1/SimpleVertexShader.hlsl
  )

set(CONTENT_FILES ${PIXELSHADER_FILES} ${VERTEXSHADER_FILES})

if (WINDOWS_PHONE8)
  set(CONTENT_FILES ${CONTENT_FILES}
    ${CMAKE_CURRENT_BINARY_DIR}/${APP_MANIFEST_NAME}
    Direct3DApp1/Assets/Tiles/FlipCycleTileLarge.png
    Direct3DApp1/Assets/Tiles/FlipCycleTileMedium.png
    Direct3DApp1/Assets/Tiles/FlipCycleTileSmall.png
    Direct3DApp1/Assets/Tiles/IconicTileMediumLarge.png
    Direct3DApp1/Assets/Tiles/IconicTileSmall.png
    Direct3DApp1/Assets/ApplicationIcon.png
    )
  # Windows Phone 8.0 needs to copy all the images.
  # It doesn't know to use relative paths.
  file(COPY
    Direct3DApp1/Assets/Tiles/FlipCycleTileLarge.png
    Direct3DApp1/Assets/Tiles/FlipCycleTileMedium.png
    Direct3DApp1/Assets/Tiles/FlipCycleTileSmall.png
    Direct3DApp1/Assets/Tiles/IconicTileMediumLarge.png
    Direct3DApp1/Assets/Tiles/IconicTileSmall.png
    Direct3DApp1/Assets/ApplicationIcon.png
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    )

elseif (NOT "${PLATFORM}" STREQUAL "DESKTOP")
  set(CONTENT_FILES ${CONTENT_FILES}
    ${CMAKE_CURRENT_BINARY_DIR}/${APP_MANIFEST_NAME}
    )

    set(ASSET_FILES ${ASSET_FILES}
    Direct3DApp1/Assets/Logo.png
    Direct3DApp1/Assets/SmallLogo.png
    Direct3DApp1/Assets/SmallLogo44x44.png
    Direct3DApp1/Assets/SplashScreen.png
    Direct3DApp1/Assets/StoreLogo.png
    )
endif()

set(STRING_FILES
  Direct3DApp1/Strings/en-US/Resources.resw)

set(RESOURCE_FILES
  ${CONTENT_FILES} ${DEBUG_CONTENT_FILES} ${RELEASE_CONTENT_FILES} ${ASSET_FILES} ${STRING_FILES}
  Direct3DApp1/Direct3DApp1_TemporaryKey.pfx)

set_property(SOURCE ${CONTENT_FILES} PROPERTY VS_DEPLOYMENT_CONTENT 1)
set_property(SOURCE ${ASSET_FILES} PROPERTY VS_DEPLOYMENT_CONTENT 1)
set_property(SOURCE ${ASSET_FILES} PROPERTY VS_DEPLOYMENT_LOCATION "Assets")
set_property(SOURCE ${STRING_FILES} PROPERTY VS_TOOL_OVERRIDE "PRIResource")
set_property(SOURCE ${DEBUG_CONTENT_FILES} PROPERTY VS_DEPLOYMENT_CONTENT $<CONFIG:Debug>)
set_property(SOURCE ${RELEASE_CONTENT_FILES} PROPERTY
  VS_DEPLOYMENT_CONTENT $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>)

set_property(SOURCE ${PIXELSHADER_FILES} PROPERTY VS_SHADER_TYPE Pixel)
set_property(SOURCE ${PIXELSHADER_FILES} PROPERTY VS_SHADER_ENTRYPOINT mainPS)
set_property(SOURCE ${PIXELSHADER_FILES} PROPERTY VS_SHADER_MODEL 4.0_level_9_3)
set_property(SOURCE ${PIXELSHADER_FILES} PROPERTY VS_SHADER_FLAGS "/DFLAGS_ADDED")
set_property(SOURCE ${PIXELSHADER_FILES} PROPERTY VS_SHADER_OUTPUT_HEADER_FILE "$(OutDir)%(Filename).h")

set_property(SOURCE ${VERTEXSHADER_FILES} PROPERTY VS_SHADER_TYPE Vertex)
set_property(SOURCE ${VERTEXSHADER_FILES} PROPERTY VS_SHADER_ENTRYPOINT mainVS)
set_property(SOURCE ${VERTEXSHADER_FILES} PROPERTY VS_SHADER_MODEL 4.0_level_9_3)
set_property(SOURCE ${VERTEXSHADER_FILES} PROPERTY VS_SHADER_FLAGS "/DFLAGS_ADDED")
set_property(SOURCE ${VERTEXSHADER_FILES} PROPERTY VS_SHADER_OUTPUT_HEADER_FILE "$(OutDir)%(Filename).h")


source_group("Source Files" FILES ${SOURCE_FILES})
source_group("Header Files" FILES ${HEADER_FILES})
source_group("Resource Files" FILES ${RESOURCE_FILES})

add_executable(${EXE_NAME} WIN32 ${SOURCE_FILES} ${HEADER_FILES} ${RESOURCE_FILES})
set_property(TARGET ${EXE_NAME} PROPERTY VS_WINRT_COMPONENT TRUE)

string(SUBSTRING "${CMAKE_SYSTEM_VERSION}" 0, 4, SHORT_VERSION)

if("${SHORT_VERSION}" STREQUAL "10.0")
  message(STATUS "Targeting Windows 10. Setting Extensions to version ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
  set_property(TARGET ${EXE_NAME} PROPERTY VS_DESKTOP_EXTENSIONS_VERSION "${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
  set_property(TARGET ${EXE_NAME} PROPERTY VS_MOBILE_EXTENSIONS_VERSION "${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
  set_property(TARGET ${EXE_NAME} PROPERTY VS_IOT_EXTENSIONS_VERSION "${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")

  # Add a reference to an SDK
  set_property(TARGET ${EXE_NAME} PROPERTY VS_SDK_REFERENCES "Microsoft.UniversalCRT.Debug, Version=${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
endif()


target_link_libraries(${EXE_NAME} d3d11)
