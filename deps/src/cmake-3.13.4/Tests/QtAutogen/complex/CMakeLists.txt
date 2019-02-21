cmake_minimum_required(VERSION 3.10)
project(Complex)
include("../AutogenTest.cmake")

# -- Test: AUTOMOC AUTORCC AUTOUIC
add_definitions(-DFOO -DSomeDefine="Barx")

# enable relaxed mode so automoc can handle all the special cases:
set(CMAKE_AUTOMOC_RELAXED_MODE TRUE)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# create an executable and two library targets, each requiring automoc:
add_library(codeeditorLib STATIC codeeditor.cpp)
add_library(privateSlot OBJECT private_slot.cpp)
# Pass Qt compiler features to targets that don't link against Qt
target_compile_features(codeeditorLib PRIVATE ${QT_COMPILE_FEATURES})
target_compile_features(privateSlot PRIVATE ${QT_COMPILE_FEATURES})

configure_file(generated_resource.qrc.in generated_resource.qrc @ONLY)
add_custom_command(
  OUTPUT generated.txt
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/generated.txt.in" "${CMAKE_CURRENT_BINARY_DIR}/generated.txt"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/generated.txt.in"
)

add_custom_target(generate_moc_input
  DEPENDS generated.txt
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/myinterface.h.in" "${CMAKE_CURRENT_BINARY_DIR}"
  COMMAND ${CMAKE_COMMAND} -E rename "${CMAKE_CURRENT_BINARY_DIR}/myinterface.h.in" "${CMAKE_CURRENT_BINARY_DIR}/myinterface.h"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/myotherinterface.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/myotherinterface.h.in" "${CMAKE_CURRENT_BINARY_DIR}/myotherinterface.h"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/myotherinterface.h.in"
)

get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT _isMultiConfig AND NOT CMAKE_GENERATOR STREQUAL Ninja)
  set(debug_srcs "$<$<CONFIG:Debug>:debug_class.cpp>" $<$<CONFIG:Debug>:debug_resource.qrc>)
  set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Debug>:TEST_DEBUG_CLASS>)
endif()

# The -no-protection option disables the generation of include guards.  Verify
# that setting the source file property has an effect by using this and
# issue an error in the preprocessor in calwidget.cpp if the include guard
# is defined.
set_source_files_properties(calwidget.ui PROPERTIES AUTOUIC_OPTIONS "-no-protection")

add_executable(QtAutogen main.cpp calwidget.cpp second_widget.cpp foo.cpp blub.cpp bar.cpp abc.cpp
               multiplewidgets.cpp
               xyz.cpp yaf.cpp gadget.cpp $<TARGET_OBJECTS:privateSlot>
               test.qrc second_resource.qrc resourcetester.cpp generated.cpp ${debug_srcs}
               ${CMAKE_CURRENT_BINARY_DIR}/generated_resource.qrc
)
set_property(TARGET QtAutogen APPEND PROPERTY AUTOGEN_TARGET_DEPENDS generate_moc_input "${CMAKE_CURRENT_BINARY_DIR}/myotherinterface.h")

add_executable(targetObjectsTest targetObjectsTest.cpp $<TARGET_OBJECTS:privateSlot>)
target_link_libraries(targetObjectsTest ${QT_LIBRARIES})

set_target_properties(
  QtAutogen codeeditorLib privateSlot targetObjectsTest
  PROPERTIES
  AUTOMOC TRUE
)


include(GenerateExportHeader)
# The order is relevant here. B depends on A, and B headers depend on A
# headers both subdirectories use CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE and we
# test that CMAKE_AUTOMOC successfully reads the include directories
# for the build interface from those targets. There has previously been
# a bug where caching of the include directories happened before
# extracting the includes to pass to moc.
add_subdirectory(Bdir)
add_subdirectory(Adir)
add_library(libC SHARED libC.cpp)
set_target_properties(libC PROPERTIES AUTOMOC TRUE)
generate_export_header(libC)
target_link_libraries(libC LINK_PUBLIC libB)
target_include_directories(libC PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set_property(TARGET libC APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR} )

target_link_libraries(QtAutogen codeeditorLib ${QT_LIBRARIES} libC)
