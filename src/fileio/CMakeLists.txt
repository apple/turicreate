project(fileio)

if(WIN32)
  SET(PLATFORM_DEPENDENCIES ws2_32)
else()
  SET(PLATFORM_DEPENDENCIES dl)
endif()


add_library(soft_hdfs INTERFACE )
target_compile_definitions(soft_hdfs INTERFACE HAS_HDFS)
target_compile_definitions(soft_hdfs INTERFACE HAS_HADOOP) # legacy

make_library(fileio
  SOURCES
    s3_api.cpp
    hdfs.cpp
    libhdfs_shim.cpp
    union_fstream.cpp
    general_fstream_source.cpp
    general_fstream_sink.cpp
    general_fstream.cpp
    cache_stream_source.cpp
    cache_stream_sink.cpp
    fixed_size_cache_manager.cpp
    temp_files.cpp
    sanitize_url.cpp
    file_download_cache.cpp
    fs_utils.cpp
    curl_downloader.cpp
    file_handle_pool.cpp
    fileio_constants.cpp
    s3_fstream.cpp
    block_cache.cpp
    set_curl_options.cpp
    get_s3_endpoint.cpp
    dmlcio/s3_filesys.cc
  REQUIRES
    curl openssl libxml2 logger pthread z cancel_serverside_ops globals process util soft_hdfs ${PLATFORM_DEPENDENCIES} network random aws-sdk-cpp
  MAC_REQUIRES
    iconv
  )
    

