
find_package(Qt4 REQUIRED)

set(CMAKE_WARN_DEPRECATED 1)

add_library(foo SHARED empty.cpp)
qt4_use_modules(foo LINK_PRIVATE Core)
