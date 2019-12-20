project(Turi)

# Handle the coremltools dependencies.

# If this variable is specified, then we pull the coremltools
#from that repo to compile in.It should just work here.
file(GLOB_RECURSE _mlmodel_files
  CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/coremltools/mlmodel/build/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/coremltools/mlmodel/src/*.cpp)

make_library(coremltools_mlmodel
  OBJECT
  SOURCES
    ${_mlmodel_files}
  REQUIRES protobuf
  EXTERNAL_VISIBILITY
  )

target_compile_options(coremltools_mlmodel INTERFACE "-w -DCoreML=__tc_coreml")


