include_directories(${CMAKE_CURRENT_BINARY_DIR})

if ("${PROJECT_SOURCE_DIR}" STREQUAL "${ANOTHER_PROJ_SOURCE_DIR}")
  set(BUILD_SHARED_LIBS 1)

  # Construct a source file outside the tree whose full path is close to
  # the path length limit.  This will cause the full path to the object
  # file in the build tree to exceed the maximum path length which will
  # test cmLocalGenerator::CreateSafeUniqueObjectFileName.
  get_filename_component(DEEPDIR
    ${OutOfSource_BINARY_DIR}/../OutOfSourceDeep/deeper ABSOLUTE)

  # Test giving the generator a custom limit.
  set(CMAKE_OBJECT_PATH_MAX 220)

  # Use a separate variable for computation.
  set(MAXPATH "${CMAKE_OBJECT_PATH_MAX}")

  # VS adds "OutOfSource/SubDir/OutOfSourceSubdir/../../../" to the
  # path of the source file for no good reason.  Reduce the length
  # limit by 46 characters to account for it.  It should still be long
  # enough to require special object file name conversion.
  if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    math(EXPR MAXPATH "${MAXPATH} - 46")
  endif()

  # Ninja imposes a maximum path component count of 30.  Permit more
  # path components in the source path.
  if(${CMAKE_GENERATOR} MATCHES "Ninja")
    math(EXPR MAXPATH "${MAXPATH} - 44")
  endif()

  # MAXPATH less 25 for last /and/deeper/simple.cxx part and small safety
  math(EXPR MAXPATH "${MAXPATH} - 25")
  string(LENGTH "${DEEPDIR}" DEEPDIR_LEN)
  while("${DEEPDIR_LEN}" LESS "${MAXPATH}")
    set(DEEPDIR ${DEEPDIR}/and/deeper)
    string(LENGTH "${DEEPDIR}" DEEPDIR_LEN)
  endwhile()
  set(DEEPSRC ${DEEPDIR}/simple.cxx)
  string(LENGTH "${DEEPSRC}" DEEPSRC_LEN)
  configure_file(simple.cxx.in ${DEEPSRC} COPYONLY)

  # Watcom WMake seems to have problems with long command lines.  Just
  # disable this part of the test until it is resolved.
  if(${CMAKE_GENERATOR} MATCHES "Watcom WMake")
    set(DEEPSRC "")
    add_definitions(-DNO_DEEPSRC)
  endif()

  add_library(testlib testlib.cxx)
  add_executable (simple simple.cxx ../simple.cxx ${DEEPSRC})
  target_link_libraries(simple testlib outlib)
endif ()

# test getting a definition from a subdir
set (WEASELS SIZZLING)

get_directory_property(incDirs INCLUDE_DIRECTORIES)
if(NOT incDirs)
   message(FATAL_ERROR "get_directory_property(INCLUDE_DIRECTORIES) returned empty list")
endif()
