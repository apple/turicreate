project(TuriExternalDependencies)

# Handle the coremltools dependencies.

# If this variable is specified, then we pull the coremltools
#from that repo to compile in.It should just work here.
file(GLOB_RECURSE _mlmodel_files
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  CONFIGURE_DEPENDS
  ${TC_COREMLTOOLS_DIR}/mlmodel/build/*.cc
  ${TC_COREMLTOOLS_DIR}/mlmodel/src/*.cpp)

make_library(coremltools_mlmodel
  OBJECT
  SOURCES
    ${_mlmodel_files}
  REQUIRES protobuf
  EXTERNAL_VISIBILITY
  )

target_compile_options(coremltools_mlmodel INTERFACE "-w")


