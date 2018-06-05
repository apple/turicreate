project(fileio_test)

make_executable(fs SOURCES fs REQUIRES fileio)
make_executable(general_fstream_test SOURCES general_fstream_test.cpp REQUIRES fileio)
make_executable(curl_dependency_test SOURCES curl_dependency_test.cpp REQUIRES fileio)
make_executable(fstream_bench SOURCES fstream_bench.cpp REQUIRES fileio)
make_boost_test(temp_file_test.cxx REQUIRES fileio)
make_boost_test(fixed_size_cache_manager_test.cxx REQUIRES fileio)
make_boost_test(cache_stream_test.cxx REQUIRES fileio)
make_boost_test(general_fstream_test.cxx REQUIRES fileio)
make_boost_test(parse_hdfs_url_test.cxx REQUIRES fileio)
make_boost_test(block_cache_test.cxx REQUIRES fileio random)

if (TC_ENABLE_REMOTEFS)
  make_boost_test(s3api_test.cxx REQUIRES fileio)
endif()

# this writes a big file
# make_executable(windows_4gb_test SOURCES windows_4gb_test.cpp REQUIRES fileio)
#
# requires s3
# make_boost_test(sanitize_url_test.cxx REQUIRES fileio)

