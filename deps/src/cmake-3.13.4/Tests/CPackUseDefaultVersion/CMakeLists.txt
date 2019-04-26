cmake_minimum_required(VERSION 3.2)
project(CPackUseProjectVersion NONE)

include(CPack)

message("CPACK_PACKAGE_VERSION=${CPACK_PACKAGE_VERSION}")
