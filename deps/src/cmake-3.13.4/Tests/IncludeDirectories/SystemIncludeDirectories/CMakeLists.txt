
cmake_minimum_required(VERSION 2.8)

project(SystemIncludeDirectories)

add_library(systemlib systemlib.cpp)
target_include_directories(systemlib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/systemlib")

add_library(upstream upstream.cpp)
target_link_libraries(upstream LINK_PUBLIC systemlib)
target_compile_options(upstream PRIVATE -Werror=unused-variable)

target_include_directories(upstream SYSTEM PUBLIC
  $<TARGET_PROPERTY:systemlib,INTERFACE_INCLUDE_DIRECTORIES>
)

add_library(config_specific INTERFACE)
if(CMAKE_GENERATOR STREQUAL "Xcode")
  # CMAKE_BUILD_TYPE does not work here for multi-config generators
  target_include_directories(config_specific SYSTEM INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/config_specific"
  )
else()
  set(testConfig ${CMAKE_BUILD_TYPE})
  target_include_directories(config_specific SYSTEM INTERFACE
    "$<$<CONFIG:${testConfig}>:${CMAKE_CURRENT_SOURCE_DIR}/config_specific>"
  )
endif()

add_library(consumer consumer.cpp)
target_link_libraries(consumer upstream config_specific)
target_compile_options(consumer PRIVATE -Werror=unused-variable)

add_library(iface IMPORTED INTERFACE)
set_property(TARGET iface PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/systemlib_header_only>"
  )

add_library(imported_consumer imported_consumer.cpp)
target_link_libraries(imported_consumer iface)
target_compile_options(imported_consumer PRIVATE -Werror=unused-variable)

add_library(imported_consumer2 imported_consumer.cpp)
target_link_libraries(imported_consumer2 imported_consumer)
target_compile_options(imported_consumer2 PRIVATE -Werror=unused-variable)

# add a target which has a relative system include
add_library(somelib imported_consumer.cpp)
target_include_directories(somelib SYSTEM PUBLIC "systemlib_header_only")
target_compile_options(somelib PRIVATE -Werror=unused-variable)

# add a target which consumes a relative system include
add_library(otherlib upstream.cpp)
target_link_libraries(otherlib PUBLIC somelib)
target_compile_options(somelib PRIVATE -Werror=unused-variable)

macro(do_try_compile error_option)
  set(TC_ARGS
    IFACE_TRY_COMPILE_${error_option}
    "${CMAKE_CURRENT_BINARY_DIR}/try_compile_iface" "${CMAKE_CURRENT_SOURCE_DIR}/imported_consumer.cpp"
    LINK_LIBRARIES iface
  )
  if (${error_option} STREQUAL WITH_ERROR)
    list(APPEND TC_ARGS COMPILE_DEFINITIONS -Werror=unused-variable)
  endif()
  try_compile(${TC_ARGS})
endmacro()

do_try_compile(NO_ERROR)
if (NOT IFACE_TRY_COMPILE_NO_ERROR)
  message(SEND_ERROR "try_compile failed with imported target.")
endif()
do_try_compile(WITH_ERROR)
if (NOT IFACE_TRY_COMPILE_WITH_ERROR)
  message(SEND_ERROR "try_compile failed with imported target with error option.")
endif()
