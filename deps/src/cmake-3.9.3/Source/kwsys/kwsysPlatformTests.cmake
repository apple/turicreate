# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing#kwsys for details.

SET(KWSYS_PLATFORM_TEST_FILE_C kwsysPlatformTestsC.c)
SET(KWSYS_PLATFORM_TEST_FILE_CXX kwsysPlatformTestsCXX.cxx)

MACRO(KWSYS_PLATFORM_TEST lang var description invert)
  IF(NOT DEFINED ${var}_COMPILED)
    MESSAGE(STATUS "${description}")
    TRY_COMPILE(${var}_COMPILED
      ${CMAKE_CURRENT_BINARY_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/${KWSYS_PLATFORM_TEST_FILE_${lang}}
      COMPILE_DEFINITIONS -DTEST_${var} ${KWSYS_PLATFORM_TEST_DEFINES} ${KWSYS_PLATFORM_TEST_EXTRA_FLAGS}
      CMAKE_FLAGS "-DLINK_LIBRARIES:STRING=${KWSYS_PLATFORM_TEST_LINK_LIBRARIES}"
      OUTPUT_VARIABLE OUTPUT)
    IF(${var}_COMPILED)
      FILE(APPEND
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
        "${description} compiled with the following output:\n${OUTPUT}\n\n")
    ELSE()
      FILE(APPEND
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "${description} failed to compile with the following output:\n${OUTPUT}\n\n")
    ENDIF()
    IF(${invert} MATCHES INVERT)
      IF(${var}_COMPILED)
        MESSAGE(STATUS "${description} - no")
      ELSE()
        MESSAGE(STATUS "${description} - yes")
      ENDIF()
    ELSE()
      IF(${var}_COMPILED)
        MESSAGE(STATUS "${description} - yes")
      ELSE()
        MESSAGE(STATUS "${description} - no")
      ENDIF()
    ENDIF()
  ENDIF()
  IF(${invert} MATCHES INVERT)
    IF(${var}_COMPILED)
      SET(${var} 0)
    ELSE()
      SET(${var} 1)
    ENDIF()
  ELSE()
    IF(${var}_COMPILED)
      SET(${var} 1)
    ELSE()
      SET(${var} 0)
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(KWSYS_PLATFORM_TEST_RUN lang var description invert)
  IF(NOT DEFINED ${var})
    MESSAGE(STATUS "${description}")
    TRY_RUN(${var} ${var}_COMPILED
      ${CMAKE_CURRENT_BINARY_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/${KWSYS_PLATFORM_TEST_FILE_${lang}}
      COMPILE_DEFINITIONS -DTEST_${var} ${KWSYS_PLATFORM_TEST_DEFINES} ${KWSYS_PLATFORM_TEST_EXTRA_FLAGS}
      OUTPUT_VARIABLE OUTPUT)

    # Note that ${var} will be a 0 return value on success.
    IF(${var}_COMPILED)
      IF(${var})
        FILE(APPEND
          ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
          "${description} compiled but failed to run with the following output:\n${OUTPUT}\n\n")
      ELSE()
        FILE(APPEND
          ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
          "${description} compiled and ran with the following output:\n${OUTPUT}\n\n")
      ENDIF()
    ELSE()
      FILE(APPEND
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "${description} failed to compile with the following output:\n${OUTPUT}\n\n")
      SET(${var} -1 CACHE INTERNAL "${description} failed to compile.")
    ENDIF()

    IF(${invert} MATCHES INVERT)
      IF(${var}_COMPILED)
        IF(${var})
          MESSAGE(STATUS "${description} - yes")
        ELSE()
          MESSAGE(STATUS "${description} - no")
        ENDIF()
      ELSE()
        MESSAGE(STATUS "${description} - failed to compile")
      ENDIF()
    ELSE()
      IF(${var}_COMPILED)
        IF(${var})
          MESSAGE(STATUS "${description} - no")
        ELSE()
          MESSAGE(STATUS "${description} - yes")
        ENDIF()
      ELSE()
        MESSAGE(STATUS "${description} - failed to compile")
      ENDIF()
    ENDIF()
  ENDIF()

  IF(${invert} MATCHES INVERT)
    IF(${var}_COMPILED)
      IF(${var})
        SET(${var} 1)
      ELSE()
        SET(${var} 0)
      ENDIF()
    ELSE()
      SET(${var} 1)
    ENDIF()
  ELSE()
    IF(${var}_COMPILED)
      IF(${var})
        SET(${var} 0)
      ELSE()
        SET(${var} 1)
      ENDIF()
    ELSE()
      SET(${var} 0)
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(KWSYS_PLATFORM_C_TEST var description invert)
  SET(KWSYS_PLATFORM_TEST_DEFINES ${KWSYS_PLATFORM_C_TEST_DEFINES})
  SET(KWSYS_PLATFORM_TEST_EXTRA_FLAGS ${KWSYS_PLATFORM_C_TEST_EXTRA_FLAGS})
  KWSYS_PLATFORM_TEST(C "${var}" "${description}" "${invert}")
  SET(KWSYS_PLATFORM_TEST_DEFINES)
  SET(KWSYS_PLATFORM_TEST_EXTRA_FLAGS)
ENDMACRO()

MACRO(KWSYS_PLATFORM_C_TEST_RUN var description invert)
  SET(KWSYS_PLATFORM_TEST_DEFINES ${KWSYS_PLATFORM_C_TEST_DEFINES})
  SET(KWSYS_PLATFORM_TEST_EXTRA_FLAGS ${KWSYS_PLATFORM_C_TEST_EXTRA_FLAGS})
  KWSYS_PLATFORM_TEST_RUN(C "${var}" "${description}" "${invert}")
  SET(KWSYS_PLATFORM_TEST_DEFINES)
  SET(KWSYS_PLATFORM_TEST_EXTRA_FLAGS)
ENDMACRO()

MACRO(KWSYS_PLATFORM_CXX_TEST var description invert)
  SET(KWSYS_PLATFORM_TEST_DEFINES ${KWSYS_PLATFORM_CXX_TEST_DEFINES})
  SET(KWSYS_PLATFORM_TEST_EXTRA_FLAGS ${KWSYS_PLATFORM_CXX_TEST_EXTRA_FLAGS})
  SET(KWSYS_PLATFORM_TEST_LINK_LIBRARIES ${KWSYS_PLATFORM_CXX_TEST_LINK_LIBRARIES})
  KWSYS_PLATFORM_TEST(CXX "${var}" "${description}" "${invert}")
  SET(KWSYS_PLATFORM_TEST_DEFINES)
  SET(KWSYS_PLATFORM_TEST_EXTRA_FLAGS)
  SET(KWSYS_PLATFORM_TEST_LINK_LIBRARIES)
ENDMACRO()

MACRO(KWSYS_PLATFORM_CXX_TEST_RUN var description invert)
  SET(KWSYS_PLATFORM_TEST_DEFINES ${KWSYS_PLATFORM_CXX_TEST_DEFINES})
  SET(KWSYS_PLATFORM_TEST_EXTRA_FLAGS ${KWSYS_PLATFORM_CXX_TEST_EXTRA_FLAGS})
  KWSYS_PLATFORM_TEST_RUN(CXX "${var}" "${description}" "${invert}")
  SET(KWSYS_PLATFORM_TEST_DEFINES)
  SET(KWSYS_PLATFORM_TEST_EXTRA_FLAGS)
ENDMACRO()

#-----------------------------------------------------------------------------
# KWSYS_PLATFORM_INFO_TEST(lang var description)
#
# Compile test named by ${var} and store INFO strings extracted from binary.
MACRO(KWSYS_PLATFORM_INFO_TEST lang var description)
  # We can implement this macro on CMake 2.6 and above.
  IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 2.6)
    SET(${var} "")
  ELSE()
    # Choose a location for the result binary.
    SET(KWSYS_PLATFORM_INFO_FILE
      ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/${var}.bin)

    # Compile the test binary.
    IF(NOT EXISTS ${KWSYS_PLATFORM_INFO_FILE})
      MESSAGE(STATUS "${description}")
      TRY_COMPILE(${var}_COMPILED
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/${KWSYS_PLATFORM_TEST_FILE_${lang}}
        COMPILE_DEFINITIONS -DTEST_${var}
          ${KWSYS_PLATFORM_${lang}_TEST_DEFINES}
          ${KWSYS_PLATFORM_${lang}_TEST_EXTRA_FLAGS}
        OUTPUT_VARIABLE OUTPUT
        COPY_FILE ${KWSYS_PLATFORM_INFO_FILE}
        )
      IF(${var}_COMPILED)
        FILE(APPEND
          ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
          "${description} compiled with the following output:\n${OUTPUT}\n\n")
      ELSE()
        FILE(APPEND
          ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
          "${description} failed to compile with the following output:\n${OUTPUT}\n\n")
      ENDIF()
      IF(${var}_COMPILED)
        MESSAGE(STATUS "${description} - compiled")
      ELSE()
        MESSAGE(STATUS "${description} - failed")
      ENDIF()
    ENDIF()

    # Parse info strings out of the compiled binary.
    IF(${var}_COMPILED)
      FILE(STRINGS ${KWSYS_PLATFORM_INFO_FILE} ${var} REGEX "INFO:[A-Za-z0-9]+\\[[^]]*\\]")
    ELSE()
      SET(${var} "")
    ENDIF()

    SET(KWSYS_PLATFORM_INFO_FILE)
  ENDIF()
ENDMACRO()
