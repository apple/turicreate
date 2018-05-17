project(Turi)

subdirs(
  toolkits
  cppipc
  dot_graph_printer
  exceptions
  external
  fault
  numerics
  fileio
  flexible_type
  generics
  globals
  image
  lambda
  lazy_eval
  logger
  ml_data
  nanosockets
  optimization
  platform
  random
  serialization
  sframe
  sframe_query_engine
  sgraph
  sketches
  startup_teardown
  table_printer
  unity
  util
  visualization
  )

if(${TC_BUILD_CAPI}) 
  message("Building C API.")
  subdirs(capi)
else()
  message("Skipping build of C APIs.")
endif()

