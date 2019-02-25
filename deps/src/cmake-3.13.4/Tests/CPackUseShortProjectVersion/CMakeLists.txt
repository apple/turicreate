cmake_minimum_required(VERSION 3.2)
project(CPackUseProjectVersion VERSION 2 LANGUAGES NONE)

include(CPack)

message("CPACK_PACKAGE_VERSION=${CPACK_PACKAGE_VERSION}")
