project(TuriExternalDependencies)

set(CMAKE_C_FLAGS_DEBUG
  "${EXTERNAL_CMAKE_C_FLAGS_DEBUG}")

set(CMAKE_CXX_FLAGS_DEBUG
  "${EXTERNAL_CMAKE_CXX_FLAGS_DEBUG}")

#3rd party source 
subdirs(
  libjson
  armadillo
  sparsehash
  protobuf
  lz4
  aws-sdk-cpp
  uuid_p
  xgboost
  )
