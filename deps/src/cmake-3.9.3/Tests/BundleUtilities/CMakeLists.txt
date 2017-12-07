cmake_minimum_required(VERSION 2.8)
project(BundleUtilities)

###### the various types of dependencies we can have

# a shared library
add_library(shared SHARED shared.cpp shared.h)

# another shared library
add_library(shared2 SHARED shared2.cpp shared2.h)


# a framework library
add_library(framework SHARED framework.cpp framework.h)
set_target_properties(framework PROPERTIES FRAMEWORK 1)

# make sure rpaths are not helping BundleUtilities or the executables
set_target_properties(shared shared2 framework PROPERTIES
                      SKIP_BUILD_RPATH 1)


######  test a Bundle application using dependencies

# a loadable module (depends on shared2)
# testbundleutils1 will load this at runtime
add_library(module1 MODULE module.cpp module.h)
set_target_properties(module1 PROPERTIES PREFIX "")
target_link_libraries(module1 shared2)

# a bundle application
add_executable(testbundleutils1 MACOSX_BUNDLE testbundleutils1.cpp)
target_link_libraries(testbundleutils1 shared framework ${CMAKE_DL_LIBS})

set_target_properties(testbundleutils1 module1 PROPERTIES
                      INSTALL_RPATH "${CMAKE_CURRENT_BINARY_DIR}/testdir1"
                      BUILD_WITH_INSTALL_RPATH 1)

# add custom target to install and test the app
add_custom_target(testbundleutils1_test  ALL
  COMMAND ${CMAKE_COMMAND}
  "-DINPUT=$<TARGET_FILE:testbundleutils1>"
  "-DMODULE=$<TARGET_FILE:module1>"
  "-DINPUTDIR=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}"
  "-DOUTPUTDIR=${CMAKE_CURRENT_BINARY_DIR}/testdir1"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/bundleutils.cmake"
  DEPENDS testbundleutils1 module1
  )

add_dependencies(testbundleutils1_test testbundleutils1)



######  test a non-Bundle application using dependencies

# a loadable module (depends on shared2)
# testbundleutils2 will load this at runtime
add_library(module2 MODULE module.cpp module.h)
set_target_properties(module2 PROPERTIES PREFIX "")
target_link_libraries(module2 shared2)

# a non-bundle application
add_executable(testbundleutils2 testbundleutils2.cpp)
target_link_libraries(testbundleutils2 shared framework ${CMAKE_DL_LIBS})

set_target_properties(testbundleutils2 module2 PROPERTIES
                      INSTALL_RPATH "${CMAKE_CURRENT_BINARY_DIR}/testdir2"
                      BUILD_WITH_INSTALL_RPATH 1)

# add custom target to install and test the app
add_custom_target(testbundleutils2_test  ALL
  COMMAND ${CMAKE_COMMAND}
  "-DINPUT=$<TARGET_FILE:testbundleutils2>"
  "-DMODULE=$<TARGET_FILE:module2>"
  "-DINPUTDIR=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}"
  "-DOUTPUTDIR=${CMAKE_CURRENT_BINARY_DIR}/testdir2"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/bundleutils.cmake"
  DEPENDS testbundleutils1 module2
  )
add_dependencies(testbundleutils2_test testbundleutils2)


if(APPLE AND NOT CMAKE_SYSTEM_VERSION VERSION_LESS 9.0)
######  Test a Bundle application using dependencies
######  and @rpaths on Mac OS X 10.5 or greater

  # a shared library
  add_library(shared-3 SHARED shared.cpp shared.h)

  # another shared library
  add_library(shared2-3 SHARED shared2.cpp shared2.h)

  # a framework library
  add_library(framework-3 SHARED framework.cpp framework.h)
  set_target_properties(framework-3 PROPERTIES FRAMEWORK 1)

  # build dependencies with @rpath install name
  set_target_properties(shared-3 shared2-3 framework-3 PROPERTIES
                        INSTALL_NAME_DIR "@rpath"
                        BUILD_WITH_INSTALL_RPATH 1)

  # a loadable module (depends on shared2)
  # testbundleutils1 will load this at runtime
  add_library(module3 MODULE module.cpp module.h)
  set_target_properties(module3 PROPERTIES PREFIX "" LINK_FLAGS "-Wl,-rpath,@loader_path/")
  target_link_libraries(module3 shared2-3)

  # a non-bundle application
  add_executable(testbundleutils3 testbundleutils3.cpp)
  target_link_libraries(testbundleutils3 shared-3 framework-3 ${CMAKE_DL_LIBS})

  set_target_properties(testbundleutils3 module3 PROPERTIES
                        LINK_FLAGS "-Wl,-rpath,@loader_path/"
                        BUILD_WITH_INSTALL_RPATH 1)

  # add custom target to install and test the app
  add_custom_target(testbundleutils3_test  ALL
    COMMAND ${CMAKE_COMMAND}
    "-DINPUT=$<TARGET_FILE:testbundleutils3>"
    "-DMODULE=$<TARGET_FILE:module3>"
    "-DINPUTDIR=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}"
    "-DOUTPUTDIR=${CMAKE_CURRENT_BINARY_DIR}/testdir3"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/bundleutils.cmake"
    DEPENDS testbundleutils3 module3
    )

  add_dependencies(testbundleutils3_test testbundleutils3)
endif()
