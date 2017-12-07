include(GenerateExportHeader)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

if (CMAKE_CXX_FLAGS MATCHES "-fvisibility=hidden")
  message(SEND_ERROR "Do not use add_compiler_export_flags before adding this directory")
endif()
if (CMAKE_CXX_FLAGS MATCHES "-fvisibility-inlines-hidden")
  message(SEND_ERROR "Do not use add_compiler_export_flags before adding this directory")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(lib_SRCS
  libshared_and_static.cpp
)

add_library(shared_variant SHARED ${lib_SRCS})
set_target_properties(shared_variant PROPERTIES DEFINE_SYMBOL SHARED_VARIANT_MAKEDLL)
add_library(static_variant ${lib_SRCS})

set(MY_CUSTOM_CONTENT "#define MY_CUSTOM_CONTENT_ADDED")

generate_export_header(shared_variant
  BASE_NAME libshared_and_static
  PREFIX_NAME MYPREFIX_
  CUSTOM_CONTENT_FROM_VARIABLE MY_CUSTOM_CONTENT
)

target_compile_definitions(static_variant PUBLIC MYPREFIX_LIBSHARED_AND_STATIC_STATIC_DEFINE)

export(TARGETS shared_variant static_variant FILE Targets.cmake)
