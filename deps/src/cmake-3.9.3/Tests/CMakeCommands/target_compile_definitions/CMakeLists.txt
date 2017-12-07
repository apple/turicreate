
cmake_minimum_required(VERSION 2.8)

project(target_compile_definitions)

add_executable(target_compile_definitions
  "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)
target_compile_definitions(target_compile_definitions
  PRIVATE MY_PRIVATE_DEFINE
  PUBLIC MY_PUBLIC_DEFINE
  INTERFACE MY_INTERFACE_DEFINE
)

add_executable(consumer
  "${CMAKE_CURRENT_SOURCE_DIR}/consumer.cpp"
)

target_compile_definitions(consumer
  PRIVATE $<TARGET_PROPERTY:target_compile_definitions,INTERFACE_COMPILE_DEFINITIONS>
  -DDASH_D_DEFINE
)

# Test no items
target_compile_definitions(consumer
  PRIVATE
)

if (CMAKE_GENERATOR MATCHES "Makefiles" OR CMAKE_GENERATOR MATCHES "Ninja")
  target_sources(consumer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/consumer.c"
  )
  target_compile_definitions(consumer
    PRIVATE
      CONSUMER_LANG_$<COMPILE_LANGUAGE>
      LANG_IS_CXX=$<COMPILE_LANGUAGE:CXX>
      LANG_IS_C=$<COMPILE_LANGUAGE:C>
  )
  target_compile_definitions(consumer
    PRIVATE -DTEST_LANG_DEFINES
  )
endif()

add_definitions(-DSOME_DEF)
add_library(imp UNKNOWN IMPORTED)
get_target_property(_res imp COMPILE_DEFINITIONS)
if (_res)
  message(SEND_ERROR "add_definitions populated the COMPILE_DEFINITIONS target property")
endif()
