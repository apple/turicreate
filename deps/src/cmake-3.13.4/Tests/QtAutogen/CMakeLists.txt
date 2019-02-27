cmake_minimum_required(VERSION 3.7)

project(QtAutogen)

# Tell find_package(Qt5) where to find Qt.
if(QT_QMAKE_EXECUTABLE)
  get_filename_component(Qt_BIN_DIR "${QT_QMAKE_EXECUTABLE}" PATH)
  get_filename_component(Qt_PREFIX_DIR "${Qt_BIN_DIR}" PATH)
  set(CMAKE_PREFIX_PATH ${Qt_PREFIX_DIR})
endif()

if (QT_TEST_VERSION STREQUAL 4)
  find_package(Qt4 REQUIRED)

  # Include this directory before using the UseQt4 file.
  add_subdirectory(defines_test)

  include(UseQt4)

  set(QT_QTCORE_TARGET Qt4::QtCore)

  macro(qtx_wrap_cpp)
    qt4_wrap_cpp(${ARGN})
  endmacro()

else()
  if (NOT QT_TEST_VERSION STREQUAL 5)
    message(SEND_ERROR "Invalid Qt version specified.")
  endif()
  find_package(Qt5Widgets REQUIRED)

  set(QT_QTCORE_TARGET Qt5::Core)

  include_directories(${Qt5Widgets_INCLUDE_DIRS})
  set(QT_LIBRARIES Qt5::Widgets)

  if(Qt5_POSITION_INDEPENDENT_CODE AND CMAKE_CXX_COMPILE_OPTIONS_PIC)
    add_definitions(${CMAKE_CXX_COMPILE_OPTIONS_PIC})
  endif()

  macro(qtx_wrap_cpp)
    qt5_wrap_cpp(${ARGN})
  endmacro()

endif()

get_property(QT_COMPILE_FEATURES TARGET ${QT_QTCORE_TARGET} PROPERTY INTERFACE_COMPILE_FEATURES)

# -- Test
# RCC only
add_executable(rccOnly rccOnly.cpp rccOnlyRes.qrc)
set_property(TARGET rccOnly PROPERTY AUTORCC ON)
target_link_libraries(rccOnly ${QT_QTCORE_TARGET})

# -- Test
# RCC empty
add_executable(rccEmpty rccEmpty.cpp rccEmptyRes.qrc)
set_property(TARGET rccEmpty PROPERTY AUTORCC ON)
target_link_libraries(rccEmpty ${QT_QTCORE_TARGET})

# -- Test
# UIC only
qtx_wrap_cpp(uicOnlyMoc uicOnlySource/uiconly.h)
add_executable(uicOnly uicOnlySource/uiconly.cpp ${uicOnlyMoc})
set_property(TARGET uicOnly PROPERTY AUTOUIC ON)
target_link_libraries(uicOnly ${QT_LIBRARIES})

# -- Test
# Add not_generated_file.qrc to the source list to get the file-level
# dependency, but don't generate a c++ file from it.  Disable the AUTORCC
# feature for this target.  This tests that qrc files in the sources don't
# have an effect on generation if AUTORCC is off.
add_library(empty STATIC empty.cpp not_generated_file.qrc)
set_target_properties(empty PROPERTIES AUTORCC OFF)
set_target_properties(empty PROPERTIES AUTOMOC TRUE)
target_link_libraries(empty no_link_language)
add_library(no_link_language STATIC empty.h)
set_target_properties(no_link_language PROPERTIES AUTOMOC TRUE)
# Pass Qt compiler features to targets that don't link against Qt
target_compile_features(no_link_language PRIVATE ${QT_COMPILE_FEATURES})
target_compile_features(empty PRIVATE ${QT_COMPILE_FEATURES})

# -- Test
# When a file listed in a .qrc file changes the target must be rebuilt
set(timeformat "%Y%j%H%M%S")
set(RCC_DEPENDS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/rccDepends")
set(RCC_DEPENDS_BIN "${CMAKE_CURRENT_BINARY_DIR}/rccDepends")
configure_file(${RCC_DEPENDS_SRC}/res1a.qrc.in ${RCC_DEPENDS_BIN}/res1.qrc COPYONLY)
configure_file(${RCC_DEPENDS_SRC}/res2a.qrc.in ${RCC_DEPENDS_BIN}/res2.qrc.in COPYONLY)
try_compile(RCC_DEPENDS
  "${RCC_DEPENDS_BIN}"
  "${RCC_DEPENDS_SRC}"
  rccDepends
  CMAKE_FLAGS "-DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}"
              "-DQT_TEST_VERSION=${QT_TEST_VERSION}"
              "-DCMAKE_PREFIX_PATH=${Qt_PREFIX_DIR}"
  OUTPUT_VARIABLE output
)
if (NOT RCC_DEPENDS)
  message(SEND_ERROR "Initial build of rccDepends failed. Output: ${output}")
endif()
# Get name and timestamp of the output binary
file(STRINGS "${RCC_DEPENDS_BIN}/target.txt" targetList)
list(GET targetList 0 rccDependsBin)
file(TIMESTAMP "${rccDependsBin}" timeBegin "${timeformat}")
# Sleep, touch regular qrc input file, rebuild and compare timestamp
execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1) # Ensure that the timestamp will change.
execute_process(COMMAND "${CMAKE_COMMAND}" -E touch "${RCC_DEPENDS_BIN}/res1/input.txt")
execute_process(COMMAND "${CMAKE_COMMAND}" --build . WORKING_DIRECTORY "${RCC_DEPENDS_BIN}" RESULT_VARIABLE result)
if (result)
  message(SEND_ERROR "Second build of rccDepends failed.")
endif()
file(TIMESTAMP "${rccDependsBin}" timeStep1 "${timeformat}")
if (NOT timeStep1 GREATER timeBegin)
  message(SEND_ERROR "File (${rccDependsBin}) should have changed in the first step!")
endif()
# Sleep, update regular qrc file, rebuild and compare timestamp
execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1) # Ensure that the timestamp will change.
configure_file(${RCC_DEPENDS_SRC}/res1b.qrc.in ${RCC_DEPENDS_BIN}/res1.qrc COPYONLY)
execute_process(COMMAND "${CMAKE_COMMAND}" --build . WORKING_DIRECTORY "${RCC_DEPENDS_BIN}" RESULT_VARIABLE result)
if (result)
  message(SEND_ERROR "Third build of rccDepends failed.")
endif()
file(TIMESTAMP "${rccDependsBin}" timeStep2 "${timeformat}")
if (NOT timeStep2 GREATER timeStep1)
  message(SEND_ERROR "File (${rccDependsBin}) should have changed in the second step!")
endif()
# Sleep, touch regular qrc newly added input file, rebuild and compare timestamp
execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1) # Ensure that the timestamp will change.
execute_process(COMMAND "${CMAKE_COMMAND}" -E touch "${RCC_DEPENDS_BIN}/res1/inputAdded.txt")
execute_process(COMMAND "${CMAKE_COMMAND}" --build . WORKING_DIRECTORY "${RCC_DEPENDS_BIN}" RESULT_VARIABLE result)
if (result)
  message(SEND_ERROR "Fourth build of rccDepends failed.")
endif()
file(TIMESTAMP "${rccDependsBin}" timeStep3 "${timeformat}")
if (NOT timeStep3 GREATER timeStep2)
  message(SEND_ERROR "File (${rccDependsBin}) should have changed in the third step!")
endif()
# Sleep, touch generated qrc input file, rebuild and compare timestamp
execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1) # Ensure that the timestamp will change.
execute_process(COMMAND "${CMAKE_COMMAND}" -E touch "${RCC_DEPENDS_BIN}/res2/input.txt")
execute_process(COMMAND "${CMAKE_COMMAND}" --build . WORKING_DIRECTORY "${RCC_DEPENDS_BIN}" RESULT_VARIABLE result)
if (result)
  message(SEND_ERROR "Fifth build of rccDepends failed.")
endif()
file(TIMESTAMP "${rccDependsBin}" timeStep4 "${timeformat}")
if (NOT timeStep4 GREATER timeStep3)
  message(SEND_ERROR "File (${rccDependsBin}) should have changed in the fourth step!")
endif()
# Sleep, update generated qrc file, rebuild and compare timestamp
execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1) # Ensure that the timestamp will change.
configure_file(${RCC_DEPENDS_SRC}/res2b.qrc.in ${RCC_DEPENDS_BIN}/res2.qrc.in COPYONLY)
execute_process(COMMAND "${CMAKE_COMMAND}" --build . WORKING_DIRECTORY "${RCC_DEPENDS_BIN}" RESULT_VARIABLE result)
if (result)
  message(SEND_ERROR "Sixth build of rccDepends failed.")
endif()
file(TIMESTAMP "${rccDependsBin}" timeStep5 "${timeformat}")
if (NOT timeStep5 GREATER timeStep4)
  message(SEND_ERROR "File (${rccDependsBin}) should have changed in the fitfh step!")
endif()
# Sleep, touch generated qrc newly added input file, rebuild and compare timestamp
execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1) # Ensure that the timestamp will change.
execute_process(COMMAND "${CMAKE_COMMAND}" -E touch "${RCC_DEPENDS_BIN}/res2/inputAdded.txt")
execute_process(COMMAND "${CMAKE_COMMAND}" --build . WORKING_DIRECTORY "${RCC_DEPENDS_BIN}" RESULT_VARIABLE result)
if (result)
  message(SEND_ERROR "Seventh build of rccDepends failed.")
endif()
file(TIMESTAMP "${rccDependsBin}" timeStep6 "${timeformat}")
if (NOT timeStep6 GREATER timeStep5)
  message(SEND_ERROR "File (${rccDependsBin}) should have changed in the sixth step!")
endif()


# -- Test
# Ensure a repeated build succeeds when a header containing a QObject changes
set(timeformat "%Y%j%H%M%S")
configure_file(mocRerun/test1a.h.in mocRerun/test1.h COPYONLY)
try_compile(MOC_RERUN
  "${CMAKE_CURRENT_BINARY_DIR}/mocRerun"
  "${CMAKE_CURRENT_SOURCE_DIR}/mocRerun"
  mocRerun
  CMAKE_FLAGS "-DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}"
              "-DQT_TEST_VERSION=${QT_TEST_VERSION}"
              "-DCMAKE_PREFIX_PATH=${Qt_PREFIX_DIR}"
  OUTPUT_VARIABLE output
)
if (NOT MOC_RERUN)
  message(SEND_ERROR "Initial build of mocRerun failed. Output: ${output}")
endif()
# Get name and timestamp of the output binary
file(STRINGS "${CMAKE_CURRENT_BINARY_DIR}/mocRerun/target1.txt" target1List)
list(GET target1List 0 binFile)
file(TIMESTAMP "${binFile}" timeBegin "${timeformat}")
# Change file content and rebuild
execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1)
configure_file(mocRerun/test1b.h.in mocRerun/test1.h COPYONLY)
execute_process(COMMAND "${CMAKE_COMMAND}" --build .
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/mocRerun"
  RESULT_VARIABLE mocRerun_result
  )
if (mocRerun_result)
  message(SEND_ERROR "Second build of mocRerun failed.")
endif()
# Compare timestamps
file(TIMESTAMP "${binFile}" timeStep1 "${timeformat}")
if (NOT timeStep1 GREATER timeBegin)
  message(SEND_ERROR "File (${binFile}) should have changed in the first step!")
endif()

# -- Test
# Test for SKIP_AUTOMOC and SKIP_AUTOGEN on an AUTOMOC enabled target
qtx_wrap_cpp(skipMocWrapMoc
  skipSource/qItemA.hpp
  skipSource/qItemB.hpp)
set(skipMocSources
  skipMoc.cpp
  skipSource/qItemA.cpp
  skipSource/qItemB.cpp
  skipSource/qItemC.cpp)
set_property(SOURCE skipSource/qItemA.cpp PROPERTY SKIP_AUTOMOC ON)
set_property(SOURCE skipSource/qItemB.cpp PROPERTY SKIP_AUTOGEN ON)
# AUTOMOC enabled only
add_executable(skipMocA ${skipMocSources} ${skipMocWrapMoc})
set_property(TARGET skipMocA PROPERTY AUTOMOC ON)
target_link_libraries(skipMocA ${QT_LIBRARIES})
# AUTOMOC and AUTOUIC enabled
add_executable(skipMocB ${skipMocSources} ${skipMocWrapMoc})
set_property(TARGET skipMocB PROPERTY AUTOMOC ON)
set_property(TARGET skipMocB PROPERTY AUTOUIC ON)
target_link_libraries(skipMocB ${QT_LIBRARIES})

# -- Test
# Test for SKIP_AUTOUIC and SKIP_AUTOGEN on an AUTOUIC enabled target
set(skipUicSources
  skipUic.cpp
  skipSource/skipUicGen.cpp
  skipSource/skipUicNoGen1.cpp
  skipSource/skipUicNoGen2.cpp
)
set_property(SOURCE skipSource/skipUicNoGen1.cpp PROPERTY SKIP_AUTOUIC ON)
set_property(SOURCE skipSource/skipUicNoGen2.cpp PROPERTY SKIP_AUTOGEN ON)
# AUTOUIC enabled
add_executable(skipUicA ${skipUicSources})
set_property(TARGET skipUicA PROPERTY AUTOUIC ON)
target_link_libraries(skipUicA ${QT_LIBRARIES})
# AUTOUIC and AUTOMOC enabled
add_executable(skipUicB ${skipUicSources})
set_property(TARGET skipUicB PROPERTY AUTOUIC ON)
set_property(TARGET skipUicB PROPERTY AUTOMOC ON)
target_link_libraries(skipUicB ${QT_LIBRARIES})

# -- Test
# Test for SKIP_AUTORCC and SKIP_AUTOGEN on an AUTORCC enabled target
set(skipRccSources
  skipRcc.cpp
  skipSource/skipRccBad1.qrc
  skipSource/skipRccBad2.qrc
  skipSource/skipRccGood.qrc
)
set_property(SOURCE skipSource/skipRccBad1.qrc PROPERTY SKIP_AUTORCC ON)
set_property(SOURCE skipSource/skipRccBad2.qrc PROPERTY SKIP_AUTOGEN ON)
# AUTORCC enabled
add_executable(skipRccA ${skipRccSources})
set_property(TARGET skipRccA PROPERTY AUTORCC ON)
target_link_libraries(skipRccA ${QT_LIBRARIES})
# AUTORCC, AUTOUIC and AUTOMOC enabled
add_executable(skipRccB ${skipRccSources})
set_property(TARGET skipRccB PROPERTY AUTORCC ON)
set_property(TARGET skipRccB PROPERTY AUTOUIC ON)
set_property(TARGET skipRccB PROPERTY AUTOMOC ON)
target_link_libraries(skipRccB ${QT_LIBRARIES})

# -- Test
# Source files with the same basename in different subdirectories
add_subdirectory(sameName)

# -- Test
# Tests AUTOMOC with generated sources
add_subdirectory(mocDepends)

# -- Test
# Tests various include moc patterns
add_subdirectory(mocIncludeStrict)

# -- Test
# Tests various include moc patterns
add_subdirectory(mocIncludeRelaxed)

# -- Test
# Tests Q_PLUGIN_METADATA json file change detection
if (NOT QT_TEST_VERSION STREQUAL 4)
  try_compile(MOC_PLUGIN
    "${CMAKE_CURRENT_BINARY_DIR}/mocPlugin"
    "${CMAKE_CURRENT_SOURCE_DIR}/mocPlugin"
    mocPlugin
    CMAKE_FLAGS "-DQT_TEST_VERSION=${QT_TEST_VERSION}"
                "-DCMAKE_PREFIX_PATH=${Qt_PREFIX_DIR}"
    OUTPUT_VARIABLE output
  )
  if (NOT MOC_PLUGIN)
    message(SEND_ERROR "Initial build of mocPlugin failed. Output: ${output}")
  endif()

  set(timeformat "%Y%j%H%M%S")
  set(mocPlugSrcDir "${CMAKE_CURRENT_SOURCE_DIR}/mocPlugin")
  set(mocPlugBinDir "${CMAKE_CURRENT_BINARY_DIR}/mocPlugin")
  find_library(plAFile "PlugA" PATHS "${mocPlugBinDir}/Debug" "${mocPlugBinDir}" NO_DEFAULT_PATH)
  find_library(plBFile "PlugB" PATHS "${mocPlugBinDir}/Debug" "${mocPlugBinDir}" NO_DEFAULT_PATH)
  find_library(plCFile "PlugC" PATHS "${mocPlugBinDir}/Debug" "${mocPlugBinDir}" NO_DEFAULT_PATH)
  find_library(plDFile "PlugD" PATHS "${mocPlugBinDir}/Debug" "${mocPlugBinDir}" NO_DEFAULT_PATH)

  file(TIMESTAMP "${plAFile}" plABefore "${timeformat}")
  file(TIMESTAMP "${plBFile}" plBBefore "${timeformat}")
  file(TIMESTAMP "${plCFile}" plCBefore "${timeformat}")
  file(TIMESTAMP "${plDFile}" plDBefore "${timeformat}")

  # Ensure that the timestamp will change and change the json files
  execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1)
  configure_file("${mocPlugSrcDir}/jsonIn/StyleD.json" "${mocPlugBinDir}/jsonFiles/StyleC.json")
  configure_file("${mocPlugSrcDir}/jsonIn/StyleC.json" "${mocPlugBinDir}/jsonFiles/sub/StyleD.json")
  execute_process(COMMAND "${CMAKE_COMMAND}" --build . WORKING_DIRECTORY "${mocPlugBinDir}")

  file(TIMESTAMP "${plAFile}" plAAfter "${timeformat}")
  file(TIMESTAMP "${plBFile}" plBAfter "${timeformat}")
  file(TIMESTAMP "${plCFile}" plCAfter "${timeformat}")
  file(TIMESTAMP "${plDFile}" plDAfter "${timeformat}")

  if (plAAfter GREATER plABefore)
    message(SEND_ERROR "file (${plAFile}) should not have changed!")
  endif()
  if (plBAfter GREATER plBBefore)
    message(SEND_ERROR "file (${plBFile}) should not have changed!")
  endif()
  if (NOT plCAfter GREATER plCBefore)
    message(SEND_ERROR "file (${plCFile}) should have changed!")
  endif()
  if (NOT plDAfter GREATER plDBefore)
    message(SEND_ERROR "file (${plDFile}) should have changed!")
  endif()

  # Test custom macro
  file(TIMESTAMP "${plCFile}" plCBefore "${timeformat}")
  file(TIMESTAMP "${plDFile}" plDBefore "${timeformat}")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1)
  configure_file("${mocPlugSrcDir}/jsonIn/StyleD.json" "${mocPlugBinDir}/jsonFiles/StyleC_Custom.json")
  configure_file("${mocPlugSrcDir}/jsonIn/StyleC.json" "${mocPlugBinDir}/jsonFiles/sub/StyleD_Custom.json")
  execute_process(COMMAND "${CMAKE_COMMAND}" --build . WORKING_DIRECTORY "${mocPlugBinDir}")
  file(TIMESTAMP "${plCFile}" plCAfter "${timeformat}")
  file(TIMESTAMP "${plDFile}" plDAfter "${timeformat}")
  if (NOT plCAfter GREATER plCBefore)
    message(SEND_ERROR "file (${plCFile}) should have changed!")
  endif()
  if (NOT plDAfter GREATER plDBefore)
    message(SEND_ERROR "file (${plDFile}) should have changed!")
  endif()

endif()

# -- Test
# Tests various .ui include directories
add_subdirectory(uicInclude)

# -- Test
# Complex test case
add_subdirectory(complex)
