enable_language(CXX)

function (use_autogen target)
  find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
  set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    PROPERTY
      Qt5Core_VERSION_MAJOR "${Qt5Core_VERSION_MAJOR}")
  set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    PROPERTY
      Qt5Core_VERSION_MINOR "${Qt5Core_VERSION_MINOR}")
  set_property(TARGET "${target}" PROPERTY AUTOMOC 1)
  set_property(TARGET "${target}" PROPERTY AUTORCC 1)
  set_property(TARGET "${target}" PROPERTY AUTOUIC 1)
endfunction ()

function (wrap_autogen target)
  use_autogen("${target}")
endfunction ()

add_executable(main empty.cpp)
wrap_autogen(main)
