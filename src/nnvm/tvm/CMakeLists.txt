set(USE_CUDA OFF)
set(USE_OPENCL OFF)
set(USE_METAL OFF)
set(USE_RPC OFF)
set(USE_GRAPH_RUNTIME ON)
set(USE_LLVM OFF)
set(USE_RTTI ON)
set(USE_MSVC_MT OFF)
set(INSTALL_DEV OFF)



set(TVM_LINKER_LIBS "")
set(TVM_RUNTIME_LINKER_LIBS "")

set(RUNTIME_SRCS
    ./src/runtime/module.cc
    ./src/runtime/workspace_pool.cc
    ./src/runtime/module_util.cc
    ./src/runtime/system_lib_module.cc
    ./src/runtime/dso_module.cc
    ./src/runtime/registry.cc
    ./src/runtime/thread_pool.cc
    ./src/runtime/cpu_device_api.cc
    ./src/runtime/rocm/rocm_module.cc
    ./src/runtime/rocm/rocm_device_api.cc
    ./src/runtime/file_util.cc
    ./src/runtime/c_dsl_api.cc
    ./src/runtime/c_runtime_api.cc)

if (USE_LLVM)
set(COMPILER_LLVM_SRCS 
    ./src/codegen/llvm/codegen_llvm.cc
    ./src/codegen/llvm/codegen_cpu.cc
    ./src/codegen/llvm/codegen_nvptx.cc
    ./src/codegen/llvm/intrin_rule_llvm.cc
    ./src/codegen/llvm/codegen_arm.cc
    ./src/codegen/llvm/llvm_common.cc
    ./src/codegen/llvm/codegen_amdgpu.cc
    ./src/codegen/llvm/llvm_module.cc)
endif()


if(USE_CUDA)
set(RUNTIME_CUDA_SRCS 
    ./src/runtime/cuda/cuda_device_api.cc
    ./src/runtime/cuda/cuda_module.cc)
endif()

if(USE_OPENCL)
set(RUNTIME_OPENCL_SRCS 
    ./src/runtime/opencl/opencl_module.cc
    ./src/runtime/opencl/opencl_device_api.cc)
endif()

if(USE_METAL)
set(RUNTIME_METAL_SRCS 
  src/runtime/metal/metal_device_api.mm
  src/runtime/metal/metal_module.mm)
endif()

if(USE_RPC)
set(RUNTIME_RPC_SRCS 
    ./src/runtime/rpc/rpc_socket_impl.cc
    ./src/runtime/rpc/rpc_device_api.cc
    ./src/runtime/rpc/rpc_event_impl.cc
    ./src/runtime/rpc/rpc_server_env.cc
    ./src/runtime/rpc/rpc_session.cc
    ./src/runtime/rpc/rpc_module.cc)
endif()

if(USE_GRAPH_RUNTIME)
set(RUNTIME_GRAPH_SRCS 
    ./src/runtime/graph/graph_runtime.cc)
endif()

make_library(tvm
  SOURCES
    ./src/schedule/auto_inline_elem_wise.cc
    ./src/schedule/schedule_dataflow_rewrite.cc
    ./src/schedule/bound.cc
    ./src/schedule/schedule_ops.cc
    ./src/schedule/message_passing.cc
    ./src/schedule/graph.cc
    ./src/schedule/schedule_lang.cc
    ./src/pass/storage_flatten.cc
    ./src/pass/lower_intrin.cc
    ./src/pass/storage_sync.cc
    ./src/pass/arg_binder.cc
    ./src/pass/ir_util.cc
    ./src/pass/combine_context_call.cc
    ./src/pass/lower_thread_allreduce.cc
    ./src/pass/inline.cc
    ./src/pass/remove_no_op.cc
    ./src/pass/ir_mutator.cc
    ./src/pass/inject_double_buffer.cc
    ./src/pass/inject_prefetch.cc
    ./src/pass/split_host_device.cc
    ./src/pass/inject_virtual_thread.cc
    ./src/pass/loop_partition.cc
    ./src/pass/simple_passes.cc
    ./src/pass/coproc_sync.cc
    ./src/pass/narrow_channel_access.cc
    ./src/pass/make_api.cc
    ./src/pass/rewrite_unsafe_select.cc
    ./src/pass/lower_tvm_builtin.cc
    ./src/pass/unroll_loop.cc
    ./src/pass/ssa.cc
    ./src/pass/vectorize_loop.cc
    ./src/pass/ir_visitor.cc
    ./src/pass/storage_access.cc
    ./src/pass/lift_attr_scope.cc
    ./src/pass/ir_deep_compare.cc
    ./src/pass/split_pipeline.cc
    ./src/pass/storage_rewrite.cc
    ./src/op/op_util.cc
    ./src/op/scan_op.cc
    ./src/op/tensorize.cc
    ./src/op/cross_thread_reduction.cc
    ./src/op/placeholder_op.cc
    ./src/op/extern_op.cc
    ./src/op/compute_op.cc
    ./src/codegen/codegen_cuda.cc
    ./src/codegen/codegen_metal.cc
    ./src/codegen/codegen.cc
    ./src/codegen/codegen_source_base.cc
    ./src/codegen/codegen_c.cc
    ./src/codegen/build_metal.cc
    ./src/codegen/codegen_opencl.cc
    ./src/codegen/build_cuda.cc
    ./src/codegen/intrin_rule_cuda.cc
    ./src/codegen/stack_vm/stack_vm_module.cc
    ./src/codegen/stack_vm/codegen_stack_vm.cc
    ./src/codegen/stack_vm/stack_vm.cc
    ./src/codegen/intrin_rule_opencl.cc
    ./src/codegen/intrin_rule.cc
    ./src/codegen/build_opencl.cc
    ./src/codegen/source_module.cc
    ./src/lang/target_info.cc
    ./src/lang/expr.cc
    ./src/lang/tensor.cc
    ./src/lang/buffer.cc
    ./src/lang/ir_operator.cc
    ./src/lang/channel.cc
    ./src/lang/lowered_func.cc
    ./src/lang/reflection.cc
    ./src/lang/ir.cc
    ./src/arithmetic/modular.cc
    ./src/arithmetic/canonical.cc
    ./src/arithmetic/detect_linear_equation.cc
    ./src/arithmetic/domain_touched.cc
    ./src/arithmetic/int_set.cc
    ./src/arithmetic/bound_deducer.cc
    ./src/api/api_codegen.cc
    ./src/api/api_arith.cc
    ./src/api/api_pass.cc
    ./src/api/api_ir.cc
    ./src/api/api_lang.cc
    ./src/api/api_base.cc
    ./src/api/dsl_api.cc
    ./src/api/api_schedule.cc
    # halide
    ./HalideIR/src/ir/IR.cpp
    ./HalideIR/src/ir/IRVisitor.cpp
    ./HalideIR/src/ir/IROperator.cpp
    ./HalideIR/src/ir/IREquality.cpp
    ./HalideIR/src/ir/IRPrinter.cpp
    ./HalideIR/src/ir/IRMutator.cpp
    ./HalideIR/src/arithmetic/Deinterleave.cpp
    ./HalideIR/src/arithmetic/Simplify.cpp
    ./HalideIR/src/arithmetic/Substitute.cpp
    ./HalideIR/src/arithmetic/ModulusRemainder.cpp
    ./HalideIR/src/arithmetic/Interval.cpp
    ./HalideIR/src/tvm/node.cpp
    ./HalideIR/src/base/Float16Opt.cpp
    ./HalideIR/src/base/Util.cpp
    ./HalideIR/src/base/Error.cpp
    ./HalideIR/src/base/Debug.cpp
    ./HalideIR/src/base/Type.cpp
    ${RUNTIME_SRCS}
    ${COMPILER_LLVM_SRCS}
    ${RUNTIME_CUDA_SRCS}
    ${RUNTIME_OPENCL_SRCS}
    ${RUNTIME_METAL_SRCS}
    ${RUNTIME_RPC_SRCS}
    ${RUNTIME_GRAPH_SRCS}
  REQUIRES
  dmlc)

target_include_directories(tvm PUBLIC "include")
target_include_directories(tvm PRIVATE "HalideIR/src")
target_include_directories(tvm PUBLIC "dlpack/include")


if(USE_CUDA)
find_package(CUDA)
# Find CUDA doesn't find all the libraries we need, add the extra ones
find_library(CUDA_CUDA_LIBRARIES cuda
  PATHS ${CUDA_TOOLKIT_ROOT_DIR}
  PATH_SUFFIXES lib lib64 targets/x86_64-linux/lib targets/x86_64-linux/lib/stubs)
find_library(CUDA_NVRTC_LIBRARIES nvrtc
  PATHS ${CUDA_TOOLKIT_ROOT_DIR}
  PATH_SUFFIXES lib lib64 targets/x86_64-linux/lib targets/x86_64-linux/lib/stubs)
  set(CUDA_CUDA_LIBRARY ${CUDA_CUDA_LIBRARIES})

  find_package(CUDA QUIET REQUIRED)
  message(STATUS "Build with CUDA support")
  target_include_directories(tvm PRIVATE ${CUDA_INCLUDE_DIRS})
  list(APPEND TVM_RUNTIME_LINKER_LIBS ${CUDA_CUDART_LIBRARY})
  list(APPEND TVM_RUNTIME_LINKER_LIBS ${CUDA_CUDA_LIBRARY})
  list(APPEND RUNTIME_SRCS ${RUNTIME_CUDA_SRCS})
  if(MSVC)
    find_library(CUDA_NVRTC_LIB nvrtc
      ${CUDA_TOOLKIT_ROOT_DIR}/lib/x64
      ${CUDA_TOOLKIT_ROOT_DIR}/lib/win32)
    list(APPEND TVM_LINKER_LIBS ${CUDA_NVRTC_LIB})
  else(MSVC)
    find_library(CUDA_NVRTC_LIB nvrtc
      ${CUDA_TOOLKIT_ROOT_DIR}/lib64
      ${CUDA_TOOLKIT_ROOT_DIR}/lib)
    list(APPEND TVM_LINKER_LIBS ${CUDA_NVRTC_LIB})
  endif(MSVC)
  target_compile_definitions(tvm PRIVATE -DTVM_CUDA_RUNTIME=1)
else(USE_CUDA)
  target_compile_definitions(tvm PRIVATE -DTVM_CUDA_RUNTIME=0)
endif(USE_CUDA)

if(USE_OPENCL)
  find_package(OPENCL QUIET REQUIRED)
  message(STATUS "Build with OpenCL support")
  include_directories(${OPENCL_INCLUDE_DIRS})
  list(APPEND TVM_RUNTIME_LINKER_LIBS ${OpenCL_LIBRARIES})
  list(APPEND RUNTIME_SRCS ${RUNTIME_OPENCL_SRCS})
  target_compile_definitions(tvm PRIVATE -DTVM_OPENCL_RUNTIME=1)
else(USE_OPENCL)
  target_compile_definitions(tvm PRIVATE -DTVM_OPENCL_RUNTIME=0)
endif(USE_OPENCL)

if(USE_METAL)
  find_package(OPENCL QUIET REQUIRED)
  message(STATUS "Build with Metal support")
  FIND_LIBRARY(METAL_LIB Metal)
  FIND_LIBRARY(FOUNDATION_LIB Foundation)
  list(APPEND TVM_RUNTIME_LINKER_LIBS ${METAL_LIB} ${FOUNDATION_LIB})
  list(APPEND RUNTIME_SRCS ${RUNTIME_METAL_SRCS})
  target_compile_definitions(tvm PRIVATE -DTVM_METAL_RUNTIME=1)
else(USE_METAL)
  target_compile_definitions(tvm PRIVATE -DTVM_METAL_RUNTIME=0)
endif(USE_METAL)


if(USE_LLVM)
  find_package(LLVM CONFIG REQUIRED)
  target_include_directories(tvm PRIVATE ${LLVM_INCLUDE_DIRS})
  target_compile_definitions(tvm PRIVATE ${LLVM_DEFINITIONS})
  set(TVM_LLVM_VERSION ${LLVM_VERSION_MAJOR}${LLVM_VERSION_MINOR})
  message(STATUS "Build with LLVM " ${LLVM_PACKAGE_VERSION})
  message(STATUS "Set TVM_LLVM_VERSION=" ${TVM_LLVM_VERSION})
  target_compile_definitions(tvm PRIVATE -DTVM_LLVM_VERSION=${TVM_LLVM_VERSION})
  target_compile_definitions(tvm PRIVATE -DDMLC_USE_FOPEN64=0)
  llvm_map_components_to_libnames(LLVM_LIBS all)
  list(REMOVE_ITEM LLVM_LIBS LTO)
  list(APPEND TVM_LINKER_LIBS ${LLVM_LIBS})
  list(APPEND COMPILER_SRCS ${COMPILER_LLVM_SRCS})
  if(NOT MSVC)
    set_property(SOURCE ${COMPILER_LLVM_SRCS} APPEND_STRING PROPERTY COMPILE_FLAGS
      "-fno-rtti -DDMLC_ENABLE_RTTI=0")
  endif()
endif(USE_LLVM)

if(NOT USE_RTTI)
  add_definitions(-DDMLC_ENABLE_RTTI=0)
endif()

target_link_libraries(tvm PUBLIC ${TVM_LINKER_LIBS} ${TVM_RUNTIME_LINKER_LIBS})
