project(image)


#/**************************************************************************/
#/*                                                                        */
#/*                             Logger Library                             */
#/*                                                                        */
#/**************************************************************************/
make_library(image_type
  SOURCES
    image_type.cpp
  REQUIRES
    logger
    EXTERNAL_VISIBILITY
)

make_executable(io_example
  SOURCES
    io_example.cpp
  REQUIRES
    image_io
)

make_library(image_io
        SOURCES
    io.cpp
    jpeg_io.cpp
    png_io.cpp
    REQUIRES
      libpng
      libjpeg
      logger
      fileio
      EXTERNAL_VISIBILITY)
#set to native if supported
check_cxx_compiler_flag(-Wno-c++11-narrowing HAS_NARROWING)

if(HAS_NARROWING)
set_property(TARGET image_io APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-c++11-narrowing")
endif()
