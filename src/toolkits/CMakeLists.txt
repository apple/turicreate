project(TuriCreate_toolkits)

# If this variable is specified, then we pull the coremltools
#from that repo to compile in.It should just work here.
file(GLOB_RECURSE _toolkits_src_files
  RELATIVE
  ${CMAKE_CURRENT_SOURCE_DIR}
  CONFIGURE_DEPENDS
  LIST_DIRECTORIES false
  *.cpp *.c *.cc *.mm
  )

make_library(toolkits
  OBJECT
  SOURCES
    ${_toolkits_src_files}
  EXTERNAL_VISIBILITY
  )


