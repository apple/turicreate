add_library(jumpShared SHARED jumpShared.cxx)

if(WIN32 OR CYGWIN)
  set(SHARED_MUST_BE_IN_EXE_DIR 1)
endif()

if(APPLE)
  set(SHARED_MUST_BE_IN_EXE_DIR 1)
endif()

if(SHARED_MUST_BE_IN_EXE_DIR)
  set(LIB_NAME
    ${CMAKE_SHARED_LIBRARY_PREFIX}jumpShared${CMAKE_SHARED_LIBRARY_SUFFIX})
  set(EXE_DIR ${Jump_BINARY_DIR}/Executable)
  if(EXECUTABLE_OUTPUT_PATH)
    set(EXE_DIR ${EXECUTABLE_OUTPUT_PATH})
  endif()
  set(LIB_DIR ${Jump_BINARY_DIR}/Library/Shared)
  if(LIBRARY_OUTPUT_PATH)
    set(LIB_DIR ${LIBRARY_OUTPUT_PATH})
  endif()
  add_custom_command(TARGET jumpShared
    POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy
    ${LIB_DIR}/${CMAKE_CFG_INTDIR}/${LIB_NAME}
    ${EXE_DIR}/${CMAKE_CFG_INTDIR}/${LIB_NAME})
endif()
