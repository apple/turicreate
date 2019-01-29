project(TuriExternalDependencies)

set(CMAKE_C_FLAGS_DEBUG
  "${EXTERNAL_CMAKE_C_FLAGS_DEBUG}")

set(CMAKE_CXX_FLAGS_DEBUG
  "${EXTERNAL_CMAKE_CXX_FLAGS_DEBUG}")

if (${CMAKE_BUILD_TYPE} MATCHES "Release")
  set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS_RELEASE})
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_RELEASE})
else()
  set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS_DEBUG})
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_DEBUG})
endif()

#3rd party source 

add_subdirectory(libjson)
add_subdirectory(armadillo)
add_subdirectory(sparsehash)
add_subdirectory(google)
add_subdirectory(lz4)
add_subdirectory(uuid_p)
add_subdirectory(xgboost)

if(NOT TC_BUILD_IOS)
  add_subdirectory(aws-sdk-cpp)
endif()
