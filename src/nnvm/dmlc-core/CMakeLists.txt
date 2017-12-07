project(dmlc C CXX)

make_library(dmlc
  SOURCES
  src/data.cc
  src/recordio.cc
  src/io/line_split.cc
  src/io/indexed_recordio_split.cc
  src/io/input_split_base.cc
  src/io/recordio_split.cc
  src/io/local_filesys.cc
  src/io.cc
  src/config.cc)

target_include_directories(dmlc PUBLIC "include")
target_compile_definitions(dmlc PRIVATE -DDMLC_USE_S3=0)
target_compile_definitions(dmlc PRIVATE -DDMLC_USE_AZURE=0)
target_compile_definitions(dmlc PRIVATE -DDMLC_USE_HDFS=0)
