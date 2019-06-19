subdirs(capi
core
external
deployment
ml
model_server
toolkits
visualization)


if(${TC_BUILD_PYTHON})
  subdirs(python)

  include_directories(${PYTHON_INCLUDE_DIRS})
  message("Python library = ${PYTHON_LIBRARIES}")

endif()

