cmake_minimum_required(VERSION 2.8.4) # new enough for CMP0017
project(AllFindModules)

# Avoid ctest truncation of output
message(STATUS "CTEST_FULL_OUTPUT")

set(ORIGINAL_MODULE_PATH "${CMAKE_MODULE_PATH}")

file(GLOB FIND_MODULES "${CMAKE_CURRENT_SOURCE_DIR}/../../../Modules/Find*.cmake" )

macro(do_find MODULE_NAME)
    message(STATUS "   Checking Find${MODULE_NAME}")
    find_package(${MODULE_NAME})
    set(CMAKE_MODULE_PATH "${ORIGINAL_MODULE_PATH}")
endmacro()

# It is only possible to use either Qt3 or Qt4 in one project.
# Since FindQt will complain if both are found we explicitly request Qt4 here
# and filter out FindQt3. FindKDE3 also depends on Qt3 and
# is therefore also blocked
set(DESIRED_QT_VERSION 4)
set(NO_QT4_MODULES "Qt3" "KDE3")

# ignore everything that has it's own test in Tests/Module/
file(GLOB OWN_TEST_MODULES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/../../Module/" "${CMAKE_CURRENT_SOURCE_DIR}/../../Module/Find*")
foreach(FIND_MODULE IN LISTS OWN_TEST_MODULES)
    string(REGEX REPLACE "^Find" "" _MOD_NAME "${FIND_MODULE}")
    list(APPEND NO_FIND_MODULES "${_MOD_NAME}")
endforeach()

# These modules are named Find*.cmake, but are nothing that works in
# find_package().
list(APPEND NO_FIND_MODULES "PackageHandleStandardArgs" "PackageMessage")

foreach(FIND_MODULE IN LISTS FIND_MODULES)
    string(REGEX REPLACE ".*/Find(.*)\\.cmake$" "\\1" MODULE_NAME "${FIND_MODULE}")

    list(FIND NO_QT4_MODULES ${MODULE_NAME} NO_QT4_INDEX)
    list(FIND NO_FIND_MODULES ${MODULE_NAME} NO_FIND_INDEX)
    if (NO_QT4_INDEX EQUAL -1 AND NO_FIND_INDEX EQUAL -1)
        do_find(${MODULE_NAME})
    endif ()

endforeach()

# Qt4 is not present, so we can check Qt3
if (NOT QT4_FOUND)
    set(DESIRED_QT_VERSION 3)
    foreach(FIND_MODULE ${NO_QT4_MODULES} "Qt")
        do_find(${FIND_MODULE})
    endforeach()
endif ()

macro(check_version_string MODULE_NAME VERSION_VAR)
    string(FIND " ${CMake_TEST_CMakeOnly.AllFindModules_NO_VERSION} " " ${MODULE_NAME} " _exclude_pos)
    if (NOT _exclude_pos EQUAL -1)
        message(STATUS "excluding check of ${VERSION_VAR}='${${VERSION_VAR}}' due to local configuration")
    elseif (${MODULE_NAME}_FOUND)
        if (DEFINED ${VERSION_VAR})
            message(STATUS "${VERSION_VAR}='${${VERSION_VAR}}'")
            if (NOT ${VERSION_VAR} MATCHES "^[0-9]")
                message(SEND_ERROR "unexpected: ${VERSION_VAR} does not begin with a decimal digit")
            endif()
            if ("${${VERSION_VAR}}" STREQUAL "")
                message(SEND_ERROR "unexpected: ${VERSION_VAR} is empty")
            endif()
            if (${VERSION_VAR} VERSION_EQUAL 0)
                message(SEND_ERROR "unexpected: ${VERSION_VAR} is VERSION_EQUAL 0")
            endif()
            if (NOT ${VERSION_VAR} VERSION_GREATER 0)
                message(SEND_ERROR "unexpected: ${VERSION_VAR} is NOT VERSION_GREATER 0")
            endif()
        else()
            message(SEND_ERROR "${MODULE_NAME}_FOUND is set but version number variable ${VERSION_VAR} is NOT DEFINED")
        endif()
    endif ()
endmacro()

# If any of these modules reported that it was found a version number should have been
# reported.

foreach(VTEST ALSA ARMADILLO BZIP2 CUPS CURL EXPAT FREETYPE GETTEXT GIT HG
        HSPELL ICOTOOL JASPER LIBLZMA LIBXML2 LIBXSLT LTTNGUST PERL PKG_CONFIG
        PostgreSQL TIFF ZLIB)
    check_version_string(${VTEST} ${VTEST}_VERSION_STRING)
endforeach()

foreach(VTEST BISON Boost CUDA DOXYGEN FLEX GIF GTK2
        HDF5 JPEG LibArchive OPENSCENEGRAPH RUBY SWIG Protobuf)
    check_version_string(${VTEST} ${VTEST}_VERSION)
endforeach()

check_version_string(PYTHONINTERP PYTHON_VERSION_STRING)
check_version_string(SUBVERSION Subversion_VERSION_SVN)
