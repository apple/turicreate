project(TuriExternalDependencies)

add_library(protobuf OBJECT
  # Don't need to pull in the compiler source files here.
  # Or any non-lite sources.
  arena.cc
  arenastring.cc
  extension_set.cc
  generated_message_util.cc
  message_lite.cc
  repeated_field.cc
  io/coded_stream.cc
  io/zero_copy_stream.cc
  io/zero_copy_stream_impl.cc
  io/zero_copy_stream_impl_lite.cc
  stubs/atomicops_internals_x86_gcc.cc
  stubs/atomicops_internals_x86_msvc.cc
  stubs/common.cc
  stubs/int128.cc
  stubs/once.cc
  stubs/status.cc
  stubs/statusor.cc
  stubs/stringpiece.cc
  stubs/stringprintf.cc
  stubs/structurally_valid.cc
  stubs/strutil.cc
  wire_format_lite.cc
)

target_compile_options(protobuf PRIVATE "-w")

# Add this in so that the protobuf namespace is globally redefined to __tc_google.
# This avoids symbol conflicts with other version of protobuf.  
# TODO does this need to go everywhere? Or should we keep protobuf as a static lib for now?
target_compile_definitions(protobuf PUBLIC google=__tc_google HAVE_PTHREAD=1)

