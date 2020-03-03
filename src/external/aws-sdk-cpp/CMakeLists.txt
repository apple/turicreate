project(aws-sdk-cpp)

set(BUILD_TESTING
    OFF
    CACHE BOOL "don't build aws-c-common test")
# set(BUILD_SHARED_LIBS OFF CACHE BOOL "don't build aws-checksums in shared
# library")
add_subdirectory(aws-c-common)

set(AWS_CHECKSUMS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/aws-checksums)

file(GLOB AWS_CHECKSUMS_COMMON_SRC "${AWS_CHECKSUMS_DIR}/source/*.c")

file(GLOB AWS_CHECKSUMS_ARCH_SRC "${AWS_CHECKSUMS_DIR}/source/intel/*.c"
     "${AWS_CHECKSUMS_DIR}/source/arm/*.c")

file(GLOB AWS_CHECKSUMS_SRC ${AWS_CHECKSUMS_COMMON_SRC}
     ${AWS_CHECKSUMS_ARCH_SRC})

add_library(aws-checksums ${AWS_CHECKSUMS_SRC})
target_include_directories(aws-checksums PUBLIC "${AWS_CHECKSUMS_DIR}/include")

set(AWS_EVENT_STREAM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/aws-c-event-stream)
file(GLOB AWS_EVENT_STREAM_SRC "${AWS_EVENT_STREAM_DIR}/source/*.c")

add_library(aws-c-event-stream ${AWS_EVENT_STREAM_SRC})
target_link_libraries(aws-c-event-stream aws-c-common)
target_link_libraries(aws-c-event-stream aws-checksums)
target_include_directories(aws-c-event-stream
                           PUBLIC "${AWS_EVENT_STREAM_DIR}/include")

set(AWS_CPP_SDK_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/aws-cpp-sdk-core)
file(GLOB AWS_SOURCE "${AWS_CPP_SDK_CORE_DIR}/source/*.cpp")
file(GLOB AWS_TINYXML2_SOURCE
     "${AWS_CPP_SDK_CORE_DIR}/source/external/tinyxml2/*.cpp")
file(GLOB CJSON_SOURCE "${AWS_CPP_SDK_CORE_DIR}/source/external/cjson/*.cpp")
file(GLOB AWS_AUTH_SOURCE "${AWS_CPP_SDK_CORE_DIR}/source/auth/*.cpp")
file(GLOB AWS_CLIENT_SOURCE "${AWS_CPP_SDK_CORE_DIR}/source/client/*.cpp")
file(GLOB AWS_INTERNAL_SOURCE "${AWS_CPP_SDK_CORE_DIR}/source/internal/*.cpp")
file(GLOB AWS_MODEL_SOURCE "${AWS_CPP_SDK_CORE_DIR}/source/aws/model/*.cpp")
file(GLOB HTTP_SOURCE "${AWS_CPP_SDK_CORE_DIR}/source/http/*.cpp")
file(GLOB HTTP_CURL_CLIENT_SOURCE
     "${AWS_CPP_SDK_CORE_DIR}/source/http/curl/*.cpp")
file(GLOB HTTP_STANDARD_SOURCE
     "${AWS_CPP_SDK_CORE_DIR}/source/http/standard/*.cpp")
file(GLOB CONFIG_SOURCE "${AWS_CPP_SDK_CORE_DIR}/source/config/*.cpp")
file(GLOB MONITORING_SOURCE "${AWS_CPP_SDK_CORE_DIR}/source/monitoring/*.cpp")
file(GLOB UTILS_SOURCE "${AWS_CPP_SDK_CORE_DIR}/source/utils/*.cpp")
file(GLOB UTILS_EVENT_SOURCE "${AWS_CPP_SDK_CORE_DIR}/source/utils/event/*.cpp")
file(GLOB UTILS_BASE64_SOURCE
     "${AWS_CPP_SDK_CORE_DIR}/source/utils/base64/*.cpp")
file(GLOB UTILS_CRYPTO_SOURCE
     "${AWS_CPP_SDK_CORE_DIR}/source/utils/crypto/*.cpp")
file(GLOB UTILS_JSON_SOURCE "${AWS_CPP_SDK_CORE_DIR}/source/utils/json/*.cpp")
file(GLOB UTILS_THREADING_SOURCE
     "${AWS_CPP_SDK_CORE_DIR}/source/utils/threading/*.cpp")
file(GLOB UTILS_XML_SOURCE "${AWS_CPP_SDK_CORE_DIR}/source/utils/xml/*.cpp")
file(GLOB UTILS_LOGGING_SOURCE
     "${AWS_CPP_SDK_CORE_DIR}/source/utils/logging/*.cpp")
file(GLOB UTILS_MEMORY_SOURCE
     "${AWS_CPP_SDK_CORE_DIR}/source/utils/memory/*.cpp")
file(GLOB UTILS_MEMORY_STL_SOURCE
     "${AWS_CPP_SDK_CORE_DIR}/source/utils/memory/stl/*.cpp")
file(GLOB UTILS_STREAM_SOURCE
     "${AWS_CPP_SDK_CORE_DIR}/source/utils/stream/*.cpp")
file(GLOB UTILS_CRYPTO_OPENSSL_SOURCE
     "${AWS_CPP_SDK_CORE_DIR}/source/utils/crypto/openssl/*.cpp")
# UNIX only
file(GLOB UTILS_AWS_CORE_PLATFORM
     "${AWS_CPP_SDK_CORE_DIR}/source/platform/linux-shared/*.cpp")
file(GLOB UTILS_AWS_CORE_NET_SOURCE
     "${AWS_CPP_SDK_CORE_DIR}/source/net/linux-shared/*.cpp")
file(GLOB UTILS_CRYPTO_FACTORY_SOURCE
     "${AWS_CPP_SDK_CORE_DIR}/source/utils/crypto/factory/*.cpp")

file(
  GLOB
  AWS_NATIVE_SDK_COMMON_SRC
  ${AWS_SOURCE}
  ${AWS_TINYXML2_SOURCE}
  ${CJSON_SOURCE}
  ${AWS_AUTH_SOURCE}
  ${AWS_CLIENT_SOURCE}
  ${AWS_INTERNAL_SOURCE}
  ${AWS_MODEL_SOURCE}
  ${HTTP_SOURCE}
  ${HTTP_CURL_CLIENT_SOURCE}
  ${UTILS_SOURCE}
  ${HTTP_STANDARD_SOURCE}
  ${HTTP_CLIENT_SOURCE}
  ${CONFIG_SOURCE}
  ${MONITORING_SOURCE}
  ${UTILS_EVENT_SOURCE}
  ${UTILS_BASE64_SOURCE}
  ${UTILS_CRYPTO_SOURCE}
  ${UTILS_JSON_SOURCE}
  ${UTILS_THREADING_SOURCE}
  ${UTILS_RETRY_SOURCE}
  ${UTILS_XML_SOURCE}
  ${UTILS_LOGGING_SOURCE}
  ${UTILS_MEMORY_SOURCE}
  ${UTILS_MEMORY_STL_SOURCE}
  ${UTILS_STREAM_SOURCE}
  ${UTILS_CRYPTO_OPENSSL_SOURCE}
  ${UTILS_AWS_CORE_NET}
  ${UTILS_AWS_CORE_PLATFORM}
  ${UTILS_AWS_CORE_NET_SOURCE}
  ${UTILS_CRYPTO_FACTORY_SOURCE})

set(AWS_CPP_SDK_S3_DIR ${CMAKE_CURRENT_SOURCE_DIR}/aws-cpp-sdk-s3)

file(GLOB AWS_CPP_SDK_S3_SOURCE "${AWS_CPP_SDK_S3_DIR}/source/*.cpp"
     "${AWS_CPP_SDK_S3_DIR}/source/model/*.cpp")

file(GLOB _aws_source_file_list ${AWS_NATIVE_SDK_COMMON_SRC}
     ${AWS_CPP_SDK_S3_SOURCE})

make_library(
  aws-sdk-cpp
  SHARED
  SHARED_ALL_DEFINED
  SOURCES
  ${_aws_source_file_list}
  REQUIRES
  curl
  openssl
  uuid)

target_link_libraries(aws-sdk-cpp PUBLIC aws-c-common)
target_link_libraries(aws-sdk-cpp PUBLIC aws-c-event-stream)

target_include_directories(aws-sdk-cpp
                           PRIVATE "${AWS_EVENT_STREAM_DIR}/include")
target_include_directories(aws-sdk-cpp PRIVATE "${AWS_CHECKSUMS_DIR}/include")
target_include_directories(aws-sdk-cpp
                           PRIVATE "${AWS_CPP_SDK_CORE_DIR}/include")
target_include_directories(aws-sdk-cpp PRIVATE "${AWS_CPP_SDK_S3_DIR}/include")

target_optional_compile_flag(aws-sdk-cpp "-Wno-delete-non-virtual-dtor")
target_optional_compile_flag(aws-sdk-cpp "-Wno-return-std-move")

target_compile_definitions(aws-sdk-cpp PRIVATE "-DAWS_CORE_EXPORTS")
target_compile_definitions(aws-sdk-cpp PUBLIC "-DAWS_CORE_USE_IMPORT_EXPORT")

target_compile_definitions(aws-sdk-cpp PRIVATE -DENABLE_CURL_CLIENT)
target_compile_definitions(aws-sdk-cpp PRIVATE -DENABLE_OPENSSL_ENCRYPTION)
target_compile_definitions(aws-sdk-cpp PRIVATE -DJSON_USE_EXCEPTION=0)
target_compile_definitions(aws-sdk-cpp PRIVATE -DAWS_SDK_VERSION_MAJOR=1)
target_compile_definitions(aws-sdk-cpp PRIVATE -DAWS_SDK_VERSION_MINOR=7)
target_compile_definitions(aws-sdk-cpp PRIVATE -DAWS_SDK_VERSION_PATCH=143)

# Set up the source file dependencies with the external libraries to make sure
# that cmake compiles OpenSSL first.  This list was generated by running the
# following command in this directory and copying and pasting the results here.
#
# git grep -o \<openssl/.*\> | sed
# 's|.*:\<\(.*\)\>|set_source_file_properties(${_aws_source_file_list}
# PROPERTIES OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/deps/local/include/\1)|g'

# The following header files are generated by the external dependencies.  This
# forces the compilation of those dependencies to complete before the above
# compilation starts.
set(_aws_generated_file_dependencies
    openssl/ossl_typ.h
    openssl/evp.h
    openssl/rand.h
    openssl/md5.h
    openssl/sha.h
    openssl/hmac.h
    openssl/err.h
    curl/curl.h)

list(TRANSFORM _aws_generated_file_dependencies
     PREPEND ${CMAKE_SOURCE_DIR}/deps//local/include/)

set_source_files_properties(
  ${_aws_source_file_list} PROPERTIES OBJECT_DEPENDS
                                      "${_aws_generated_file_dependencies}")
