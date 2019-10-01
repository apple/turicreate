
find_package(Qt4 REQUIRED)
find_package(Qt5Core REQUIRED)

add_executable(mainexe main.cpp)
target_link_libraries(mainexe Qt4::QtCore Qt5::Core)
