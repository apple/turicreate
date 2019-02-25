cmake_minimum_required(VERSION 3.9)

include(ExternalProject)

# Verify COMMAND keyword is recognised after various *_COMMAND options
ExternalProject_Add(multiCommand
  DOWNLOAD_COMMAND  "${CMAKE_COMMAND}" -E echo "download 1"
           COMMAND  "${CMAKE_COMMAND}" -E echo "download 2"
  UPDATE_COMMAND    "${CMAKE_COMMAND}" -E echo "update 1"
         COMMAND    "${CMAKE_COMMAND}" -E echo "update 2"
  PATCH_COMMAND     "${CMAKE_COMMAND}" -E echo "patch 1"
        COMMAND     "${CMAKE_COMMAND}" -E echo "patch 2"
  CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E echo "configure 1"
            COMMAND "${CMAKE_COMMAND}" -E echo "configure 2"
  BUILD_COMMAND     "${CMAKE_COMMAND}" -E echo "build 1"
        COMMAND     "${CMAKE_COMMAND}" -E echo "build 2"
  TEST_COMMAND      "${CMAKE_COMMAND}" -E echo "test 1"
       COMMAND      "${CMAKE_COMMAND}" -E echo "test 2"
  INSTALL_COMMAND   "${CMAKE_COMMAND}" -E echo "install 1"
          COMMAND   "${CMAKE_COMMAND}" -E echo "install 2"
)

# Workaround for issue 17229 (missing dependency between update and patch steps)
ExternalProject_Add_StepTargets(multiCommand NO_DEPENDS update)
ExternalProject_Add_StepDependencies(multiCommand patch multiCommand-update)

# Force all steps to be re-run by removing timestamps from any previous run
ExternalProject_Get_Property(multiCommand STAMP_DIR)
file(REMOVE_RECURSE "${STAMP_DIR}")
file(MAKE_DIRECTORY "${STAMP_DIR}")
