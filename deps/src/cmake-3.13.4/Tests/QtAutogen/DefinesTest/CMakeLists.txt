cmake_minimum_required(VERSION 3.10)
project(DefinesTest)

# Qt4 only definitions test
if(NOT QT_TEST_VERSION EQUAL 4)
  message(ERROR "Invalid Qt test version. This test is for Qt4 only.")
endif()

find_package(Qt4 REQUIRED)

add_executable(DefinesTest defines_test.cpp)
set_target_properties(DefinesTest PROPERTIES AUTOMOC TRUE)
target_link_libraries(DefinesTest Qt4::QtGui)
