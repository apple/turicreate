cmake_minimum_required(VERSION 2.8)
project(ExternalProjectTest NONE)

include(ExternalProject)

# Test ExternalProject, especially with checkouts from VCS

find_package(CVS)
find_package(Subversion)
find_package(Git)
find_package(Hg)

option(ExternalProjectTest_USE_FOLDERS "Enable folder grouping in IDEs." ON)
if(ExternalProjectTest_USE_FOLDERS)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
else()
  set_property(GLOBAL PROPERTY USE_FOLDERS OFF)
endif()

set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER
  "CMakePredefinedTargets-in-ExternalProjectTest")

set(base "${CMAKE_BINARY_DIR}/CMakeExternals")
set(binary_base "${base}/Build")
set_property(DIRECTORY PROPERTY EP_BASE ${base})
set_property(DIRECTORY PROPERTY EP_STEP_TARGETS configure build test)

add_custom_target(NonExternalProjectTarget
  COMMAND ${CMAKE_COMMAND} -E echo NonExternalProjectTarget)

# Empty projects that test all the known ExternalProject_Add argument key words:
#
set(proj AAA-TestAlphabetization)
ExternalProject_Add(${proj}
  BUILD_COMMAND ""
  CONFIGURE_COMMAND ""
  DOWNLOAD_COMMAND ""
  INSTALL_COMMAND ""
)

set(proj ZZZ-TestAlphabetization)
ExternalProject_Add(${proj}
  BUILD_COMMAND ""
  CONFIGURE_COMMAND ""
  DOWNLOAD_COMMAND ""
  INSTALL_COMMAND ""
)

set(proj TargetNameSameAsFolder)
ExternalProject_Add(${proj}
  BUILD_COMMAND ""
  CONFIGURE_COMMAND ""
  DOWNLOAD_COMMAND ""
  INSTALL_COMMAND ""
)
set_property(TARGET ${proj} PROPERTY FOLDER "${proj}")

set(proj MinimalNoOpProject)
ExternalProject_Add(${proj}
  BUILD_COMMAND ""
  CONFIGURE_COMMAND ""
  DOWNLOAD_COMMAND ""
  INSTALL_COMMAND ""
)

set(proj EmptyNoOpProject)
ExternalProject_Add(${proj}
  BUILD_COMMAND ""
  CMAKE_ARGS ""
  CONFIGURE_COMMAND ""
  CVS_REPOSITORY ""
  CVS_MODULE ""
  CVS_TAG ""
  DEPENDS "MinimalNoOpProject" NonExternalProjectTarget
  DOWNLOAD_COMMAND ""
  DOWNLOAD_NO_PROGRESS 1
  INSTALL_COMMAND ""
  PATCH_COMMAND ""
  STEP_TARGETS install update
  SVN_REPOSITORY ""
  SVN_REVISION ""
  SVN_USERNAME ""
  SVN_PASSWORD ""
  SVN_TRUST_CERT 1
  TEST_COMMAND ""
  TIMEOUT ""
  URL ""
  URL_MD5 ""
  UPDATE_COMMAND ""
)
set_property(TARGET ${proj} PROPERTY FOLDER "")

set(proj NoExtractLogDownload)
ExternalProject_Add(${proj}
  URL ${CMAKE_CURRENT_SOURCE_DIR}/gitrepo.tgz
  DOWNLOAD_NO_EXTRACT 1
  LOG_DOWNLOAD 1
  BUILD_COMMAND ""
  CONFIGURE_COMMAND ""
  INSTALL_COMMAND ""
)


# CVS-based tests:
#
set(do_cvs_tests 0)

if(CVS_EXECUTABLE)
  set(do_cvs_tests 1)
endif()

if(do_cvs_tests AND NOT UNIX)
  if("${CVS_EXECUTABLE}" MATCHES "cygwin")
    message(STATUS "No ExternalProject cvs tests with cygwin cvs.exe outside cygwin!")
    set(do_cvs_tests 0)
  endif()
endif()

if(do_cvs_tests)
  # Unzip/untar the CVS repository in our source folder so that other
  # projects below may use it to test CVS args of ExternalProject_Add
  #
  set(proj SetupLocalCVSRepository)
  set(local_cvs_repo "${CMAKE_CURRENT_BINARY_DIR}/LocalRepositories/CVS")
  ExternalProject_Add(${proj}
    SOURCE_DIR ${local_cvs_repo}
    URL ${CMAKE_CURRENT_SOURCE_DIR}/cvsrepo.tgz
    URL_MD5 55fc85825ffdd9ed2597123c68b79f7e
    BUILD_COMMAND ""
    CONFIGURE_COMMAND "${CVS_EXECUTABLE}" --version
    INSTALL_COMMAND ""
  )
  set_property(TARGET ${proj}
    PROPERTY FOLDER "SetupRepos/Local/Deeply/Nested/For/Testing")

  # The MSYS cvs tool interprets "c:/" as a "machine:" name for SSH.
  # Detect the MSYS cvs and convert the repo path to an MSYS path.
  if(WIN32)
    if(EXISTS "${CVS_EXECUTABLE}")
      file(STRINGS "${CVS_EXECUTABLE}" cvs_is_msys LIMIT_COUNT 1 REGEX "[Mm][Ss][Yy][Ss]")
      if(cvs_is_msys)
        message(STATUS "'${CVS_EXECUTABLE}' is from MSYS (contains '${cvs_is_msys}')")
        string(REGEX REPLACE "^([A-Za-z]):" "/\\1" local_cvs_repo "${local_cvs_repo}")
      endif()
    endif()
  endif()

  # CVS by date stamp:
  #
  set(proj TutorialStep1-CVS-20090626)
  ExternalProject_Add(${proj}
    CVS_REPOSITORY "${local_cvs_repo}"
    CVS_MODULE "TutorialStep1"
    CVS_TAG "-D2009-06-26 16:50:00 UTC"
    UPDATE_COMMAND ""
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    INSTALL_COMMAND ""
    DEPENDS "SetupLocalCVSRepository"
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "CVS")

  # CVS by tag:
  #
  set(proj TutorialStep1-CVS-testtag1)
  ExternalProject_Add(${proj}
    CVS_REPOSITORY "${local_cvs_repo}"
    CVS_MODULE "TutorialStep1"
    CVS_TAG -rtesttag1
    UPDATE_COMMAND ""
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    INSTALL_COMMAND ""
    DEPENDS "SetupLocalCVSRepository"
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "CVS")

  # Live CVS / HEAD (no CVS_TAG):
  #
  set(proj TutorialStep1-CVS-HEAD)
  ExternalProject_Add(${proj}
    CVS_REPOSITORY "${local_cvs_repo}"
    CVS_MODULE "TutorialStep1"
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    INSTALL_COMMAND ""
    DEPENDS "SetupLocalCVSRepository"
    DEPENDS "EmptyNoOpProject"
    DEPENDS "TutorialStep1-CVS-20090626"
    DEPENDS "TutorialStep1-CVS-testtag1"
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "CVS")
endif()


# SVN-based tests:
#
set(do_svn_tests 0)

if(Subversion_SVN_EXECUTABLE)
  set(do_svn_tests 1)
endif()

# Only do svn tests with svn >= version 1.2
#
if(do_svn_tests)
  if(Subversion_VERSION_SVN VERSION_LESS 1.2)
    message(STATUS "No ExternalProject svn tests with svn client less than version 1.2")
    set(do_svn_tests 0)
  endif()
endif()

# Only do svn tests in cygwin/cygwin or not-cygwin/not-cygwin arrangements:
#
if(do_svn_tests)
  if(CMAKE_CURRENT_BINARY_DIR MATCHES "cygdrive/" AND
     NOT "${Subversion_SVN_EXECUTABLE}" MATCHES "cygwin")
    message(STATUS "No ExternalProject svn tests with non-cygwin svn client in a /cygdrive based build")
    set(do_svn_tests 0)
  endif()
endif()

if(do_svn_tests)
  # Unzip/untar the SVN repository in our source folder so that other
  # projects below may use it to test SVN args of ExternalProject_Add
  #
  set(proj SetupLocalSVNRepository)
  set(local_svn_repo "${CMAKE_CURRENT_BINARY_DIR}/LocalRepositories/SVN")
  set(local_svn_repo_url "file:///${local_svn_repo}/TutorialStep1")
  ExternalProject_Add(${proj}
    SOURCE_DIR ${local_svn_repo}
    URL ${CMAKE_CURRENT_SOURCE_DIR}/svnrepo.tgz
    URL_MD5 2f468be4ed1fa96377fca0cc830819c4
    BUILD_COMMAND ""
    CONFIGURE_COMMAND "${Subversion_SVN_EXECUTABLE}" --version
    INSTALL_COMMAND ""
  )
  set_property(TARGET ${proj}
    PROPERTY FOLDER "SetupRepos/Local/Deeply/Nested/For/Testing")

  # SVN by date stamp:
  #
  set(proj TutorialStep1-SVN-20090626)
  ExternalProject_Add(${proj}
    SVN_REPOSITORY "${local_svn_repo_url}"
    SVN_REVISION "-r{2009-06-26 16:50:00 +0000}"
    UPDATE_COMMAND ""
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    INSTALL_COMMAND ""
    DEPENDS "SetupLocalSVNRepository"
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "SVN")

  # SVN by revision number:
  #
  set(proj TutorialStep1-SVN-r2)
  ExternalProject_Add(${proj}
    SVN_REPOSITORY "${local_svn_repo_url}"
    SVN_REVISION "-r2"
    UPDATE_COMMAND ""
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    INSTALL_COMMAND ""
    DEPENDS "SetupLocalSVNRepository"
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "SVN")

  # Live SVN / trunk (no SVN_REVISION):
  #
  set(proj TutorialStep1-SVN-trunk)
  ExternalProject_Add(${proj}
    SVN_REPOSITORY "${local_svn_repo_url}"
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    INSTALL_COMMAND ""
    DEPENDS "SetupLocalSVNRepository"
    LOG_DOWNLOAD 1
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "SVN")
endif()


set(do_git_tests 0)

if(GIT_EXECUTABLE)
  set(do_git_tests 1)

  execute_process(
    COMMAND "${GIT_EXECUTABLE}" --version
    OUTPUT_VARIABLE ov
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  string(REGEX REPLACE "^git version (.+)$" "\\1" git_version "${ov}")
  message(STATUS "git_version='${git_version}'")

  if(git_version VERSION_LESS 1.6.5)
    message(STATUS "No ExternalProject git tests with git client less than version 1.6.5")
    set(do_git_tests 0)
  endif()
endif()


if(do_git_tests)
  set(local_git_repo "../../LocalRepositories/GIT")

  # Unzip/untar the git repository in our source folder so that other
  # projects below may use it to test git args of ExternalProject_Add
  #
  set(proj SetupLocalGITRepository)
  ExternalProject_Add(${proj}
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/LocalRepositories/GIT
    URL ${CMAKE_CURRENT_SOURCE_DIR}/gitrepo.tgz
    BUILD_COMMAND ""
    CONFIGURE_COMMAND "${GIT_EXECUTABLE}" --version
    INSTALL_COMMAND ""
  )
  set_property(TARGET ${proj}
    PROPERTY FOLDER "SetupRepos/Local/Deeply/Nested/For/Testing")

  # git by commit id:
  #
  set(proj TutorialStep1-GIT-byhash)
  ExternalProject_Add(${proj}
    GIT_REPOSITORY "${local_git_repo}"
    GIT_TAG d1970730310fe8bc07e73f15dc570071f9f9654a
    UPDATE_COMMAND ""
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    INSTALL_COMMAND ""
    DEPENDS "SetupLocalGITRepository"
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "GIT")

  # git by explicit branch/tag name:
  #
  set(proj TutorialStep1-GIT-bytag)
  ExternalProject_Add(${proj}
    GIT_REPOSITORY "${local_git_repo}"
    GIT_TAG "origin/master"
    UPDATE_COMMAND ""
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    INSTALL_COMMAND ""
    DEPENDS "SetupLocalGITRepository"
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "GIT")

  # Live git / master (no GIT_TAG):
  #
  set(proj TutorialStep1-GIT-master)
  ExternalProject_Add(${proj}
    GIT_REPOSITORY "${local_git_repo}"
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    INSTALL_COMMAND ""
    DEPENDS "SetupLocalGITRepository"
    LOG_UPDATE 1
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "GIT")

  # Live git / master (no GIT_TAG), but shallow
  #
  set(proj TutorialStep1-GIT-shallow-master)
  ExternalProject_Add(${proj}
    GIT_REPOSITORY "${local_git_repo}"
    GIT_SHALLOW 1
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    INSTALL_COMMAND ""
    DEPENDS "SetupLocalGITRepository"
    LOG_UPDATE 1
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "GIT")

  # Live git / master (no GIT_TAG), but git config flags
  #
  # The `git clone --config` parameter has been introduced in Git 1.7.7
  if(NOT git_version VERSION_LESS 1.7.7)
    set(proj TutorialStep1-GIT-config)
    ExternalProject_Add(${proj}
      GIT_REPOSITORY "${local_git_repo}"
      GIT_CONFIG core.eol=lf core.autocrlf=input
      CMAKE_GENERATOR "${CMAKE_GENERATOR}"
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
      INSTALL_COMMAND ""
      DEPENDS "SetupLocalGITRepository"
      LOG_UPDATE 1
    )
    set_property(TARGET ${proj} PROPERTY FOLDER "GIT")
  endif()

  # git by explicit branch/tag with empty submodule list
  #
  set(proj TutorialStep1-GIT-bytag-withsubmodules)
  ExternalProject_Add(${proj}
    GIT_REPOSITORY "${local_git_repo}"
    GIT_TAG "origin/master"
    GIT_SUBMODULES ""
    UPDATE_COMMAND ""
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    INSTALL_COMMAND ""
    DEPENDS "SetupLocalGITRepository"
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "GIT")

  # Unzip/untar the git repository in our source folder so that other
  # projects below may use it to test git args of ExternalProject_Add
  #
  set(proj SetupLocalGITRepositoryWithSubmodules)
  ExternalProject_Add(${proj}
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/LocalRepositories/GIT-with-submodules
    URL ${CMAKE_CURRENT_SOURCE_DIR}/gitrepo-sub.tgz
    BUILD_COMMAND ""
    CONFIGURE_COMMAND "${GIT_EXECUTABLE}" --version
    INSTALL_COMMAND ""
  )
  set_property(TARGET ${proj}
    PROPERTY FOLDER "SetupRepos/Local/Deeply/Nested/For/Testing")

  set(local_git_repo "../../LocalRepositories/GIT-with-submodules")

  set(proj TS1-GIT-no-GIT_SUBMODULES)
  ExternalProject_Add(${proj}
    GIT_REPOSITORY "${local_git_repo}"
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
               -DWITH_m1:BOOL=ON
               -DWITH_m2:BOOL=ON
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    DEPENDS "SetupLocalGITRepository"
            "SetupLocalGITRepositoryWithSubmodules"
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "GIT")

  set(proj TS1-GIT-empty-GIT_SUBMODULES)
  ExternalProject_Add(${proj}
    GIT_REPOSITORY "${local_git_repo}"
    GIT_SUBMODULES ""
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
               -DWITH_m1:BOOL=ON
               -DWITH_m2:BOOL=ON
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    DEPENDS "SetupLocalGITRepository"
            "SetupLocalGITRepositoryWithSubmodules"
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "GIT")

  set(proj TS1-GIT-some-GIT_SUBMODULES)
  ExternalProject_Add(${proj}
    GIT_REPOSITORY "${local_git_repo}"
    GIT_SUBMODULES "m/m1"
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
               -DWITH_m1:BOOL=ON
               -DWITH_m2:BOOL=OFF
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    DEPENDS "SetupLocalGITRepository"
            "SetupLocalGITRepositoryWithSubmodules"
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "GIT")

endif()

set(do_hg_tests 0)

if(HG_EXECUTABLE)
  set(do_hg_tests 1)
endif()

if(do_hg_tests AND NOT UNIX)
  if("${HG_EXECUTABLE}" MATCHES "cygwin")
    message(STATUS "No ExternalProject hg tests with cygwin hg outside cygwin!")
    set(do_hg_tests 0)
  endif()
endif()

if(do_hg_tests)
  set(local_hg_repo "../../LocalRepositories/HG")

  # Unzip/untar the hg repository in our source folder so that other
  # projects below may use it to test hg args of ExternalProject_Add
  #
  set(proj SetupLocalHGRepository)
  ExternalProject_Add(${proj}
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/LocalRepositories/HG
    URL ${CMAKE_CURRENT_SOURCE_DIR}/hgrepo.tgz
    BUILD_COMMAND ""
    CONFIGURE_COMMAND "${HG_EXECUTABLE}" --version
    INSTALL_COMMAND ""
  )
  set_property(TARGET ${proj}
    PROPERTY FOLDER "SetupRepos/Local/Deeply/Nested/For/Testing")


  # hg by commit id:
  #
  set(proj TutorialStep1-HG-byhash)
  ExternalProject_Add(${proj}
    HG_REPOSITORY "${local_hg_repo}"
    HG_TAG dd2ce38a6b8a
    UPDATE_COMMAND ""
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    INSTALL_COMMAND ""
    DEPENDS "SetupLocalHGRepository"
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "HG")

  # hg by explicit branch/tag name:
  #
  set(proj TutorialStep1-HG-bytag)
  ExternalProject_Add(${proj}
    HG_REPOSITORY "${local_hg_repo}"
    HG_TAG "default"
    UPDATE_COMMAND ""
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    INSTALL_COMMAND ""
    DEPENDS "SetupLocalHGRepository"
  )
  set_property(TARGET ${proj} PROPERTY FOLDER "HG")

  # Live hg / tip (no HG_TAG):
  #
  # Mercurial 2.1 does not distinguish an empty pull from a failed pull,
  # so do not run the test with that version.
  if(NOT "${HG_VERSION_STRING}" STREQUAL "2.1")
    set(proj TutorialStep1-HG-tip)
    ExternalProject_Add(${proj}
      HG_REPOSITORY "${local_hg_repo}"
      CMAKE_GENERATOR "${CMAKE_GENERATOR}"
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
      INSTALL_COMMAND ""
      DEPENDS "SetupLocalHGRepository"
      LOG_UPDATE 1
    )
    set_property(TARGET ${proj} PROPERTY FOLDER "HG")
  endif()
endif()


# Test the testable built/installed products:
#
enable_testing()


# Do at least a smoke test of a built executable from each
# project's build directory...
#
# BuildTree tests:
#
if(do_cvs_tests)
  add_test(TutorialStep1-CVS-20090626-BuildTreeTest
    "${binary_base}/TutorialStep1-CVS-20090626/Tutorial" 4)

  add_test(TutorialStep1-CVS-testtag1-BuildTreeTest
    "${binary_base}/TutorialStep1-CVS-testtag1/Tutorial" 64)

  add_test(TutorialStep1-CVS-HEAD-BuildTreeTest
    "${binary_base}/TutorialStep1-CVS-HEAD/Tutorial" 81)
endif()

if(do_svn_tests)
  add_test(TutorialStep1-SVN-20090626-BuildTreeTest
    "${binary_base}/TutorialStep1-SVN-20090626/Tutorial" 100)

  add_test(TutorialStep1-SVN-r2-BuildTreeTest
    "${binary_base}/TutorialStep1-SVN-r2/Tutorial" 99)

  add_test(TutorialStep1-SVN-trunk-BuildTreeTest
    "${binary_base}/TutorialStep1-SVN-trunk/Tutorial" 98)
endif()

if(do_git_tests)
  add_test(TutorialStep1-GIT-byhash
    "${binary_base}/TutorialStep1-GIT-byhash/Tutorial" 100)

  add_test(TutorialStep1-GIT-bytag
    "${binary_base}/TutorialStep1-GIT-bytag/Tutorial" 99)

  add_test(TutorialStep1-GIT-shallow-master
    "${binary_base}/TutorialStep1-GIT-shallow-master/Tutorial" 98)

  add_test(TutorialStep1-GIT-master
    "${binary_base}/TutorialStep1-GIT-master/Tutorial" 98)
endif()


message(STATUS "do_cvs_tests='${do_cvs_tests}'")
message(STATUS "do_svn_tests='${do_svn_tests}'")
message(STATUS "do_git_tests='${do_git_tests}' GIT_EXECUTABLE='${GIT_EXECUTABLE}'")
message(STATUS "do_hg_tests='${do_hg_tests}'   HG_EXECUTABLE='${HG_EXECUTABLE}'")


# Test if log works when the first arguments of *_COMMAND is "COMMAND".
set(proj ExternalProject-no-log)
set(download_cmd "")
set(patch_cmd "")
set(update_cmd "")
set(configure_cmd "")
set(build_cmd "")
set(install_cmd "")

ExternalProject_Add(${proj}
  DOWNLOAD_COMMAND "${download_cmd}" COMMAND "${CMAKE_COMMAND}" -E echo "download"
  PATCH_COMMAND "${patch_cmd}" COMMAND "${CMAKE_COMMAND}" -E echo "patch"
  UPDATE_COMMAND "${update_cmd}" COMMAND "${CMAKE_COMMAND}" -E echo "update"
  CONFIGURE_COMMAND "${configure_cmd}" COMMAND "${CMAKE_COMMAND}" -E echo "configure"
  BUILD_COMMAND "${build_cmd}" COMMAND "${CMAKE_COMMAND}" -E echo "build"
  INSTALL_COMMAND "${install_cmd}" COMMAND "${CMAKE_COMMAND}" -E echo "install"
  )

set(proj ExternalProject-log)
ExternalProject_Add(${proj}
  DOWNLOAD_COMMAND "${download_cmd}" COMMAND "${CMAKE_COMMAND}" -E echo "download"
  PATCH_COMMAND "${patch_cmd}" COMMAND "${CMAKE_COMMAND}" -E echo "patch"
  UPDATE_COMMAND "${update_cmd}" COMMAND "${CMAKE_COMMAND}" -E echo "update"
  CONFIGURE_COMMAND "${configure_cmd}" COMMAND "${CMAKE_COMMAND}" -E echo "configure"
  BUILD_COMMAND "${build_cmd}" COMMAND "${CMAKE_COMMAND}" -E echo "build"
  INSTALL_COMMAND "${install_cmd}" COMMAND "${CMAKE_COMMAND}" -E echo "install"
  LOG_DOWNLOAD 1
  LOG_PATCH 1
  LOG_UPDATE 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
  LOG_INSTALL 1
  )
