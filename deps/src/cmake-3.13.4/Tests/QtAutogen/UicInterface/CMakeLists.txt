cmake_minimum_required(VERSION 3.10)
project(UicInterface)
include("../AutogenTest.cmake")

set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# BEGIN Upstream

set(CMAKE_VERBOSE_MAKEFILE ON)

add_library(KI18n klocalizedstring.cpp)
target_link_libraries(KI18n ${QT_QTCORE_TARGET})

set(autouic_options
  -tr tr2$<$<NOT:$<BOOL:$<TARGET_PROPERTY:NO_KUIT_SEMANTIC>>>:x>i18n
)
if (NOT Qt5Widgets_VERSION VERSION_LESS 5.3.0)
  list(APPEND autouic_options -include klocalizedstring.h)
endif()

set_property(TARGET KI18n APPEND PROPERTY
  INTERFACE_AUTOUIC_OPTIONS ${autouic_options}
)

set(domainProp $<TARGET_PROPERTY:TRANSLATION_DOMAIN>)
set(nameLower $<LOWER_CASE:$<MAKE_C_IDENTIFIER:$<TARGET_PROPERTY:NAME>>>)
set(domain_logic
  $<$<BOOL:${domainProp}>:${domainProp}>$<$<NOT:$<BOOL:${domainProp}>>:${nameLower}>
)
set_property(TARGET KI18n APPEND PROPERTY
  INTERFACE_COMPILE_DEFINITIONS "TRANSLATION_DOMAIN=${domain_logic}"
)

# END upstream

get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_isMultiConfig)
set(INC_DIR "include_$<CONFIG>" )
else()
set(INC_DIR "include" )
endif()

add_library(LibWidget libwidget.cpp)
target_link_libraries(LibWidget KI18n ${QT_QTGUI_TARGET})
set_property(TARGET LibWidget PROPERTY NO_KUIT_SEMANTIC ON)
set_property(TARGET LibWidget PROPERTY TRANSLATION_DOMAIN customdomain)

add_library(MyWidget mywidget.cpp)
target_link_libraries(MyWidget KI18n ${QT_QTGUI_TARGET})

add_executable(QtAutoUicInterface main.cpp)
target_compile_definitions(QtAutoUicInterface
  PRIVATE
    UI_LIBWIDGET_H="${CMAKE_CURRENT_BINARY_DIR}/LibWidget_autogen/${INC_DIR}/ui_libwidget.h"
    UI_MYWIDGET_H="${CMAKE_CURRENT_BINARY_DIR}/MyWidget_autogen/${INC_DIR}/ui_mywidget.h"
)
