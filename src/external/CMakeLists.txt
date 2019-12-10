project(Turi)

# Suppress all warnings in all external code.
add_compile_options("-w")


# Zlib is a core dependency; it should go first.
add_subdirectory(zlib)
add_subdirectory(libjson)
add_subdirectory(libjpeg)
add_subdirectory(libpng)
add_subdirectory(Eigen)
add_subdirectory(sparsehash)
add_subdirectory(google)
add_subdirectory(bzip2)
add_subdirectory(lz4)
add_subdirectory(uuid_p)
add_subdirectory(xgboost)
add_subdirectory(coremltools_wrap)
add_subdirectory(boost)
add_subdirectory(nanomsg)

if(${TC_BUILD_REMOTEFS})
  add_subdirectory(aws-sdk-cpp)
endif()
