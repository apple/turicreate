project(fileio)

if(WIN32)
  SET(PLATFORM_DEPENDENCIES ws2_32)
else()
  SET(PLATFORM_DEPENDENCIES dl)
endif()

if(TC_ENABLE_REMOTEFS)
  add_library(soft_hdfs INTERFACE )
  target_compile_definitions(soft_hdfs INTERFACE HAS_HDFS)
  target_compile_definitions(soft_hdfs INTERFACE HAS_HADOOP) # legacy
  set(FILEIO_REMOTE_FS_SOURCES
      s3_api.cpp
      hdfs.cpp
      libhdfs_shim.cpp
      s3_fstream.cpp
      set_curl_options.cpp
      get_s3_endpoint.cpp
      dmlcio/s3_filesys.cc
      file_download_cache.cpp
  )
set(FILEIO_REMOTE_FS_REQUIRES curl openssl aws-sdk-cpp soft_hdfs)
endif()


make_library(fileio OBJECT
  SOURCES
    ${FILEIO_REMOTE_FS_SOURCES}
    union_fstream.cpp
    general_fstream_source.cpp
    general_fstream_sink.cpp
    general_fstream.cpp
    cache_stream_source.cpp
    cache_stream_sink.cpp
    fixed_size_cache_manager.cpp
    temp_files.cpp
    sanitize_url.cpp
    fs_utils.cpp
    curl_downloader.cpp
    file_handle_pool.cpp
    fileio_constants.cpp
    block_cache.cpp
  REQUIRES
    ${FILEIO_REMOTE_FS_REQUIRES} libxml2 logger pthread z cancel_serverside_ops globals process util ${PLATFORM_DEPENDENCIES} network random
  MAC_REQUIRES
    iconv
)

target_compile_options(fileio PRIVATE -Ddmlc=__tc_dmlc)

