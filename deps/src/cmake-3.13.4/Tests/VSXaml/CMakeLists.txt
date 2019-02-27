cmake_minimum_required(VERSION 3.2)
project(VSXaml)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(SOURCE_FILES
  App.xaml.cpp
  MainPage.xaml.cpp
  pch.cpp
  )

set(HEADER_FILES
  App.xaml.h
  MainPage.xaml.h
  pch.h
  )

set(XAML_FILES
  App.xaml
  MainPage.xaml
  )

set(ASSET_FILES
    Assets/Logo.scale-100.png
    Assets/SmallLogo.scale-100.png
    Assets/SplashScreen.scale-100.png
    Assets/StoreLogo.scale-100.png
    )

set(CONTENT_FILES
    Package.appxmanifest
    )

set(RESOURCE_FILES
  ${CONTENT_FILES} ${ASSET_FILES}
  VSXaml_TemporaryKey.pfx)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set_property(SOURCE ${CONTENT_FILES} PROPERTY VS_DEPLOYMENT_CONTENT 1)
set_property(SOURCE ${ASSET_FILES} PROPERTY VS_DEPLOYMENT_CONTENT 1)
set_property(SOURCE ${ASSET_FILES} PROPERTY VS_DEPLOYMENT_LOCATION "Assets")

set_property(SOURCE "App.xaml" PROPERTY VS_XAML_TYPE "ApplicationDefinition")

source_group("Source Files" FILES ${SOURCE_FILES})
source_group("Header Files" FILES ${HEADER_FILES})
source_group("Resource Files" FILES ${RESOURCE_FILES})
source_group("Xaml Files" FILES ${XAML_FILES})

add_executable(VSXaml WIN32 ${SOURCE_FILES} ${HEADER_FILES} ${RESOURCE_FILES} ${XAML_FILES})
set_property(TARGET VSXaml PROPERTY VS_WINRT_COMPONENT TRUE)
