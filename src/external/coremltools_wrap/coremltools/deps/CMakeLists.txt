cmake_minimum_required(VERSION 3.0.0)
project(deps)

set(protobuf_BUILD_TESTS OFF CACHE BOOL "Build tests")

add_subdirectory(protobuf/cmake
	EXCLUDE_FROM_ALL
)

set_property(TARGET libprotobuf
	PROPERTY POSITION_INDEPENDENT_CODE ON
)

set_property(TARGET libprotobuf-lite
	PROPERTY POSITION_INDEPENDENT_CODE ON
)
