project(nnvm C CXX)

subdirs(dmlc-core)
subdirs(tvm)

make_library(nnvm
  SOURCES
  src/top/nn/nn.cc
  src/top/nn/pooling.cc
  src/top/nn/convolution.cc
  src/top/tensor/broadcast.cc
  src/top/tensor/elemwise.cc
  src/top/tensor/transform.cc
  src/top/tensor/reduce.cc
  src/core/node.cc
  src/core/op.cc
  src/core/graph.cc
  src/core/symbolic.cc
  src/core/pass.cc
  src/pass/saveload_json.cc
  src/pass/order_mutation.cc
  src/pass/print_graph_ir.cc
  src/pass/plan_memory.cc
  src/pass/gradient.cc
  src/pass/place_device.cc
  src/pass/infer_shape_type.cc
  src/compiler/precompute_prune.cc
  src/compiler/fold_scale_axis.cc
  src/compiler/simplify_inference.cc
  src/compiler/layout_transform.cc
  src/compiler/compile_engine.cc
  src/compiler/graph_hash.cc
  src/compiler/graph_fuse.cc
  src/compiler/packed_func_ext.cc
  src/compiler/graph_runtime.cc
  src/c_api/c_api_symbolic.cc
  src/c_api/c_api_graph.cc
  src/c_api/c_api_error.cc
  REQUIRES
  dmlc)

# nnvm includes
target_include_directories(nnvm PRIVATE BEFORE "include")
target_include_directories(nnvm PRIVATE "tvm/include")
target_include_directories(nnvm PRIVATE "tvm/dlpack/include")
target_include_directories(nnvm PRIVATE "tvm/HalideIR/src")
target_compile_definitions(nnvm PRIVATE -DNNVM_EXPORTS)
target_include_directories(nnvm PUBLIC "include")

subdirs(tests)
