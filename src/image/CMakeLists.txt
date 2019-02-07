project(image)


#/**************************************************************************/
#/*                                                                        */
#/*                             Logger Library                             */
#/*                                                                        */
#/**************************************************************************/
add_library(image_type OBJECT
  image_type.cpp
  image_util_impl.cpp
)
add_dependencies(image_type boost)

make_executable(io_example
  SOURCES
    io_example.cpp
  REQUIRES
    unity_shared
)

add_library(image_io OBJECT
  io.cpp
  jpeg_io.cpp
  png_io.cpp
)
add_dependencies(image_io boost)

#set to native if supported
check_cxx_compiler_flag(-Wno-c++11-narrowing HAS_NARROWING)

if(HAS_NARROWING)
  target_compile_options(image_io PRIVATE "-Wno-c++11-narrowing")
endif()
