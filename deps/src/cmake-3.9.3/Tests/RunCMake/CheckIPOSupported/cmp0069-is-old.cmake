project(${RunCMake_TEST} LANGUAGES C CXX)

cmake_policy(SET CMP0069 OLD)

include(CheckIPOSupported)
check_ipo_supported()
