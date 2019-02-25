
set(SWIG_EXECUTABLE "swig")
set(SWIG_DIR "/swig")
include(UseSWIG)

swig_add_library(example LANGUAGE python TYPE MODULE SOURCES example.i)

get_property(prefix TARGET ${SWIG_MODULE_example_REAL_NAME} PROPERTY PREFIX)
message(STATUS "PREFIX='${prefix}'")
message(STATUS "TARGET NAME='${SWIG_MODULE_example_REAL_NAME}'")
