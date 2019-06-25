project(sframe_query_engine)

make_library(sframe_query_engine OBJECT
   SOURCES
      planning/optimizations/optimization_transforms.cpp
      planning/optimization_engine.cpp
      planning/planner_node.cpp
      planning/planner.cpp
      execution/subplan_executor.cpp
      execution/execution_node.cpp
      execution/query_context.cpp
      operators/operator_properties.cpp
      operators/operator_transformations.cpp
      algorithm/sort.cpp
      algorithm/sort_and_merge.cpp
      algorithm/groupby_aggregate.cpp
      algorithm/ec_sort.cpp
      algorithm/ec_permute.cpp
      query_engine_lock.cpp
   REQUIRES
      sframe flexible_type pylambda dot_graph_printer
   EXTERNAL_VISIBILITY
)
