enable_language(CXX)
set(CMAKE_CXX_CPPLINT "${PSEUDO_CPPLINT}" --error)
add_executable(main main.cxx)
