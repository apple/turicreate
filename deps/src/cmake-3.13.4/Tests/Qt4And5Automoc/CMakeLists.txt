cmake_minimum_required(VERSION 2.8.12)

project(Qt4And5Automoc)

if (QT_REVERSE_FIND_ORDER)
  find_package(Qt5Core REQUIRED)
  find_package(Qt4 REQUIRED)
else()
  find_package(Qt4 REQUIRED)
  find_package(Qt5Core REQUIRED)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

macro(generate_main_file VERSION)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/main.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/main_qt${VERSION}.cpp" COPYONLY)
  file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/main_qt${VERSION}.cpp"
    "#include \"main_qt${VERSION}.moc\"\n"
  )
endmacro()

generate_main_file(4)
generate_main_file(5)

add_executable(qt4_exe "${CMAKE_CURRENT_BINARY_DIR}/main_qt4.cpp")
target_link_libraries(qt4_exe Qt4::QtCore)
add_executable(qt5_exe "${CMAKE_CURRENT_BINARY_DIR}/main_qt5.cpp")
target_link_libraries(qt5_exe Qt5::Core)
