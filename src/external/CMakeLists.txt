project(TuriExternalDependencies)

set(CMAKE_C_FLAGS_DEBUG
  "${EXTERNAL_CMAKE_C_FLAGS_DEBUG}")

set(CMAKE_CXX_FLAGS_DEBUG
  "${EXTERNAL_CMAKE_CXX_FLAGS_DEBUG}")

# Suppress all warnings in all external code.
add_compile_options("-w")

#3rd party source 

add_subdirectory(libjson)
add_subdirectory(Eigen)
add_subdirectory(sparsehash)
add_subdirectory(google)
add_subdirectory(lz4)
add_subdirectory(uuid_p)
add_subdirectory(xgboost)
add_subdirectory(coremltools_wrap)

if(NOT TC_BUILD_IOS)
  add_subdirectory(aws-sdk-cpp)
endif()
