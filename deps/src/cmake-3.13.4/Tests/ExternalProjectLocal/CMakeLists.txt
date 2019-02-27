cmake_minimum_required(VERSION 2.8)
project(ExternalProjectLocalTest NONE)

include(ExternalProject)

# Test ExternalProject with local projects

option(ExternalProjectTest_USE_FOLDERS "Enable folder grouping in IDEs." ON)
if(ExternalProjectTest_USE_FOLDERS)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
else()
  set_property(GLOBAL PROPERTY USE_FOLDERS OFF)
endif()

set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER
  "CMakePredefinedTargets-in-ExternalProjectTest")

set(base "${CMAKE_BINARY_DIR}/Ext")
set(binary_base "${base}/Build")
set_property(DIRECTORY PROPERTY EP_BASE ${base})
set_property(DIRECTORY PROPERTY EP_STEP_TARGETS configure build test)

if(NOT DEFINED can_build_tutorial_step5)
  set(can_build_tutorial_step5 1)

  # The ExternalProject builds of Tutorial Step5 cannot be built
  # correctly 2nd and later times in an in-source build...
  # (because the CMakeCache.txt from the real in-source build of
  # the Tests/Tutorial/Step5 directory gets copied when we do
  # the "source directory copy" step... but it still refers to
  # its original path which yields a configure error.) So:
  #
  if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    set(can_build_tutorial_step5 0)
  endif()
endif()

# Local DIR:
#
if(can_build_tutorial_step5)
  set(proj TutorialStep5-Local)
  ExternalProject_Add(${proj}
    URL "${CMAKE_CURRENT_SOURCE_DIR}/../Tutorial/Step5"
    CMAKE_CACHE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    CMAKE_ARGS -G ${CMAKE_GENERATOR} <SOURCE_DIR>
    TEST_BEFORE_INSTALL 1
    LOG_INSTALL 1
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "Local")
  ExternalProject_Get_Property(${proj} install_dir)
  set(TutorialStep5_install_dir ${install_dir})

  set(proj TutorialStep5-Local-TestAfterInstall)
  ExternalProject_Add(${proj}
    URL "${CMAKE_CURRENT_SOURCE_DIR}/../Tutorial/Step5"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -G ${CMAKE_GENERATOR} <SOURCE_DIR>
    CMAKE_CACHE_DEFAULT_ARGS -DUSE_MYMATH:BOOL=OFF
    TEST_AFTER_INSTALL 1
    LOG_TEST 1
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "Local")

  set(proj TutorialStep5-Local-TestExcludeFromMainBefore)
  ExternalProject_Add(${proj}
    URL "${CMAKE_CURRENT_SOURCE_DIR}/../Tutorial/Step5"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>  -G ${CMAKE_GENERATOR} <SOURCE_DIR>
    CMAKE_CACHE_DEFAULT_ARGS -DUSE_MYMATH:BOOL=OFF
    TEST_BEFORE_INSTALL 1
    TEST_EXCLUDE_FROM_MAIN 1
    STEP_TARGETS test
    LOG_TEST 1
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "Local")

  set(proj TutorialStep5-Local-TestExcludeFromMainAfter)
  ExternalProject_Add(${proj}
    URL "${CMAKE_CURRENT_SOURCE_DIR}/../Tutorial/Step5"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>  -G ${CMAKE_GENERATOR} <SOURCE_DIR>
    CMAKE_CACHE_DEFAULT_ARGS -DUSE_MYMATH:BOOL=OFF
    TEST_AFTER_INSTALL 1
    TEST_EXCLUDE_FROM_MAIN 1
    STEP_TARGETS test
    LOG_TEST 1
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "Local")

endif()


# Local TAR:
#
set(proj TutorialStep1-LocalTAR)
ExternalProject_Add(${proj}
  URL "${CMAKE_CURRENT_SOURCE_DIR}/Step1.tar"
  URL_MD5 a87c5b47c0201c09ddfe1d5738fdb1e3
  LIST_SEPARATOR ::
  PATCH_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/Step1Patch.cmake
  CMAKE_GENERATOR "${CMAKE_GENERATOR}"
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
             -DTEST_LIST:STRING=A::B::C
  INSTALL_COMMAND ""
  LOG_CONFIGURE 1
)
set_property(TARGET ${proj} PROPERTY FOLDER "Local/TAR")

set(proj TutorialStep1-LocalNoDirTAR)
ExternalProject_Add(${proj}
  URL "${CMAKE_CURRENT_SOURCE_DIR}/Step1NoDir.tar"
  URL_MD5 d09e3d370c5c908fa035c30939ee438e
  LIST_SEPARATOR @@
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -G ${CMAKE_GENERATOR} <SOURCE_DIR>
             -DTEST_LIST:STRING=1@@2@@3
  INSTALL_COMMAND ""
)
set_property(TARGET ${proj} PROPERTY FOLDER "Local/TAR")
ExternalProject_Add_Step(${proj} mypatch
  COMMAND ${CMAKE_COMMAND} -E echo "This is a custom external project step."
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/Step1Patch.cmake
  WORKING_DIRECTORY <SOURCE_DIR>
  DEPENDEES download
  DEPENDERS configure
  )


# Local TGZ:
#
set(proj TutorialStep1-LocalTGZ)
ExternalProject_Add(${proj}
  URL "${CMAKE_CURRENT_SOURCE_DIR}/Step1.tgz"
  URL_MD5 38c648e817339c356f6be00eeed79bd0
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -G ${CMAKE_GENERATOR} <SOURCE_DIR>
  INSTALL_COMMAND ""
  LOG_BUILD 1
  UPDATE_DISCONNECTED 1
)
set_property(TARGET ${proj} PROPERTY FOLDER "Local/TGZ")

set(proj TutorialStep1-LocalNoDirTGZ)
ExternalProject_Add(${proj}
  URL "${CMAKE_CURRENT_SOURCE_DIR}/Step1NoDir.tgz"
  URL_HASH SHA256=496229e2a5ed620a37c385ad9406004a18026beab8b55dd2c4565d4b7f1d5383
  CMAKE_GENERATOR "${CMAKE_GENERATOR}"
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  INSTALL_COMMAND ""
)
set_property(TARGET ${proj} PROPERTY FOLDER "Local/TGZ")


# Local BZ2:
#
# (The bz2 tests are here just to verify that the bz2 decompression is executed
#  during a test suite run... The configure and build commands are set to
#  nothing to make the test quicker. To make this more complete, I should add
#  a diff between this and the TGZ source tree since that one does build...)
#
set(proj TutorialStep1-LocalBZ2)
ExternalProject_Add(${proj}
  URL "${CMAKE_CURRENT_SOURCE_DIR}/Step1.tar.bz2"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)
set_property(TARGET ${proj} PROPERTY FOLDER "Local/BZ2")

set(proj TutorialStep1-LocalNoDirBZ2)
ExternalProject_Add(${proj}
  URL "${CMAKE_CURRENT_SOURCE_DIR}/Step1NoDir.tar.bz2"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)
set_property(TARGET ${proj} PROPERTY FOLDER "Local/BZ2")


# Local ZIP:
#
# (The zip tests are here just to verify that the zip decompression is executed
#  during a test suite run... The configure and build commands are set to
#  nothing to make the test quicker. To make this more complete, I should add
#  a diff between this and the TGZ source tree since that one does build...)
#
set(proj TutorialStep1-LocalZIP)
ExternalProject_Add(${proj}
  URL "${CMAKE_CURRENT_SOURCE_DIR}/Step1.zip"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)
set_property(TARGET ${proj} PROPERTY FOLDER "Local/ZIP")

set(proj TutorialStep1-LocalNoDirZIP)
ExternalProject_Add(${proj}
  URL "${CMAKE_CURRENT_SOURCE_DIR}/Step1NoDir.zip"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)
set_property(TARGET ${proj} PROPERTY FOLDER "Local/ZIP")


# Test the testable built/installed products:
#
enable_testing()


# Do at least a smoke test of a built executable from each
# project's build directory...
#
# BuildTree tests:
#
if(can_build_tutorial_step5)
  add_test(TutorialStep5-Local-BuildTreeTest
    "${binary_base}/TutorialStep5-Local/Tutorial" 42)
  set_property(TEST TutorialStep5-Local-BuildTreeTest
    APPEND PROPERTY LABELS Step5 BuildTree)
endif()

add_test(TutorialStep1-LocalTAR-BuildTreeTest
  "${binary_base}/TutorialStep1-LocalTAR/EP-Tutorial" 36)
set_property(TEST TutorialStep1-LocalTAR-BuildTreeTest
  APPEND PROPERTY LABELS TAR)

add_test(TutorialStep1-LocalNoDirTAR-BuildTreeTest
  "${binary_base}/TutorialStep1-LocalNoDirTAR/EP-Tutorial" 25)

add_test(TutorialStep1-LocalTGZ-BuildTreeTest
  "${binary_base}/TutorialStep1-LocalTGZ/Tutorial" 16)
set_property(TEST TutorialStep1-LocalTGZ-BuildTreeTest
  APPEND PROPERTY LABELS TGZ)

add_test(TutorialStep1-LocalNoDirTGZ-BuildTreeTest
  "${binary_base}/TutorialStep1-LocalNoDirTGZ/Tutorial" 9)

# InstallTree tests:
#
if(can_build_tutorial_step5)
  add_test(TutorialStep5-InstallTreeTest
    "${TutorialStep5_install_dir}/bin/Tutorial" 49)
  set_property(TEST TutorialStep5-InstallTreeTest
    APPEND PROPERTY LABELS Step5 InstallTree)
endif()


message(STATUS "can_build_tutorial_step5='${can_build_tutorial_step5}'")
