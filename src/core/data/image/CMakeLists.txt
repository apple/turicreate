project(image)


#/**************************************************************************/
#/*                                                                        */
#/*                             Logger Library                             */
#/*                                                                        */
#/**************************************************************************/
make_library(image_type OBJECT
  SOURCES
    image_type.cpp
    image_util_impl.cpp
  REQUIRES
    logger
    image_io
  EXTERNAL_VISIBILITY
)

make_executable(io_example
  SOURCES
    io_example.cpp
  REQUIRES
    unity_shared_for_testing
)

make_library(image_io OBJECT
  SOURCES
    io.cpp
    jpeg_io.cpp
    png_io.cpp
  REQUIRES
    libpng
    libjpeg
    logger
    fileio
  EXTERNAL_VISIBILITY
)

#set to native if supported
check_cxx_compiler_flag(-Wno-c++11-narrowing HAS_NARROWING)

if(HAS_NARROWING)
  target_compile_options(image_io PRIVATE "-Wno-c++11-narrowing")
endif()
