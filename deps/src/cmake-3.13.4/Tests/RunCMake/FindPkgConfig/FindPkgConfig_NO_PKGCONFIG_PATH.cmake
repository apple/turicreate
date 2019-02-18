# Prepare environment and variables
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH FALSE)
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/pc-foo")
if(WIN32)
    set(PKG_CONFIG_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}\\dummy-pkg-config.bat")
    set(ENV{CMAKE_PREFIX_PATH} "${CMAKE_CURRENT_SOURCE_DIR}\\pc-bar;X:\\this\\directory\\should\\not\\exist\\in\\the\\filesystem")
    set(ENV{PKG_CONFIG_PATH} "C:\\baz")
else()
    set(PKG_CONFIG_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/dummy-pkg-config.sh")
    set(ENV{CMAKE_PREFIX_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/pc-bar:/this/directory/should/not/exist/in/the/filesystem")
    set(ENV{PKG_CONFIG_PATH} "/baz")
endif()


find_package(PkgConfig)

if(WIN32)
  set(expected_path "C:\\baz")
else()
  set(expected_path "/baz")
endif()


pkg_check_modules(FOO "${expected_path}")

if(NOT FOO_FOUND)
  message(FATAL_ERROR "Expected PKG_CONFIG_PATH: \"${expected_path}\".")
endif()



set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)

pkg_check_modules(BAR "${expected_path}" NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH)

if(NOT BAR_FOUND)
  message(FATAL_ERROR "Expected PKG_CONFIG_PATH: \"${expected_path}\".")
endif()
