project(Turi)

if(${TC_BUILD_CAPI}) 
  message("Building C API.")
  subdirs(capi)
else()
  message("Skipping build of C APIs.")
endif()

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
  logger
  ml_data
  nanosockets
  objc
  optimization
  platform
  random
  serialization
  sframe
  sframe_query_engine
  sgraph
  sketches
  stack_trace
  startup_teardown
  table_printer
  unity
  util
  vega_renderer
  visualization
  )
