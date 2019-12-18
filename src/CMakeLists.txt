
subdirs(
  external
  capi
  core
  deployment
  ml
  toolkits
  visualization
  model_server)


if(TC_BUILD_PYTHON)
  subdirs(python)

  include_directories(${PYTHON_INCLUDE_DIRS})
  message("Python library = ${PYTHON_LIBRARIES}")

endif()

