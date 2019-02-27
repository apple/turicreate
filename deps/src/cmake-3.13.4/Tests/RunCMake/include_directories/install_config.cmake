
enable_language(CXX)

add_executable(foo empty.cpp)
install(TARGETS foo EXPORT fooTargets DESTINATION . INCLUDES DESTINATION include/$<CONFIGURATION>)
install(EXPORT fooTargets DESTINATION lib/cmake)
