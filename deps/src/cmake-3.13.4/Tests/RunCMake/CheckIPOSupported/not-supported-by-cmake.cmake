project(${RunCMake_TEST} LANGUAGES C)
set(_CMAKE_C_IPO_SUPPORTED_BY_CMAKE NO)
check_ipo_supported()
