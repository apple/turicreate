
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

add_library(libA SHARED libA.cpp)
target_link_libraries(libA LINK_PUBLIC ${QT_QTCORE_TARGET})
generate_export_header(libA)
