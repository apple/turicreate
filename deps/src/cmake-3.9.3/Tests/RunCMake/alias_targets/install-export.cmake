
enable_language(CXX)

add_library(foo empty.cpp)

add_library(alias ALIAS foo)

install(TARGETS alias EXPORT theTargets DESTINATION prefix)
install(EXPORT theTargets DESTINATION lib/cmake)
