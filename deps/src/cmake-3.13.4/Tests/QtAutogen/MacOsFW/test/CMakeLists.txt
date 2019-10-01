include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
include_directories(SYSTEM
  ${Qt5Core_INCLUDE_DIRS}
  ${Qt5Widgets_INCLUDE_DIRS}
)

set(testname AutomocMacosFWLib)
add_executable(${testname} testMacosFWLib.cpp)
set_target_properties(${testname} PROPERTIES AUTOMOC TRUE)
target_link_libraries(${testname}
  Qt5::Core
  Qt5::Widgets
  Qt5::Test
  macos_fw_lib
)
