
find_package(Qt4 REQUIRED)

set(CMAKE_AUTOUIC ON)

set(CMAKE_DEBUG_TARGET_PROPERTIES AUTOUIC_OPTIONS)

add_library(KI18n INTERFACE)
set_property(TARGET KI18n APPEND PROPERTY
  INTERFACE_AUTOUIC_OPTIONS -tr ki18n
)

add_library(OtherI18n INTERFACE)
set_property(TARGET OtherI18n APPEND PROPERTY
  INTERFACE_AUTOUIC_OPTIONS -tr otheri18n
)

add_library(LibWidget empty.cpp)
target_link_libraries(LibWidget KI18n OtherI18n Qt4::QtGui)
