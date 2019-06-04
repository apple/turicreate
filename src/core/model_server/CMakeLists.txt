project(TuriCreate_model_server)

# If this variable is specified, then we pull the coremltools
#from that repo to compile in.It should just work here.
file(GLOB_RECURSE _model_server_src_files
  RELATIVE
  LIST_DIRECTORIES false
  CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}
  *.cpp
  )

make_library(core
  OBJECT
  SOURCES
    ${_model_server_src_files}
  EXTERNAL_VISIBILITY
  )
