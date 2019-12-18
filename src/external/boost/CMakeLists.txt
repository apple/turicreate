project(Turi)

set(__boost_src_dir "${CMAKE_CURRENT_SOURCE_DIR}/boost_1_68_0/")

set(__boost_lib_dir "${__boost_src_dir}/libs/")

file(GLOB_RECURSE boost_src_files
  CONFIGURE_DEPENDS
  ${__boost_lib_dir}/system/src/*.cpp
  ${__boost_lib_dir}/random/src/*.cpp
  ${__boost_lib_dir}/date_time/src/*.cpp
  ${__boost_lib_dir}/chrono/src/*.cpp
  ${__boost_lib_dir}/variant/src/*.cpp
  ${__boost_lib_dir}/filesystem/src/*.cpp
  ${__boost_lib_dir}/iostreams/src/*.cpp
  ${__boost_lib_dir}/iostreams/src/*.cpp
  ${__boost_lib_dir}/regex/src/*.cpp
  ${__boost_lib_dir}/test/src/*.cpp
  ${__boost_lib_dir}/thread/src/future.cpp
  ${__boost_lib_dir}/thread/src/tss_null.cpp
  ${__boost_lib_dir}/thread/src/pthread/once_atomic.cpp
  ${__boost_lib_dir}/thread/src/pthread/thread.cpp
)

list(FILTER boost_src_files EXCLUDE REGEX ".*/test/.*")
list(FILTER boost_src_files EXCLUDE REGEX ".*/win32/.*")
list(FILTER boost_src_files EXCLUDE REGEX ".*/lzma.cpp")

message("Boost libraries: ${boost_src_files}")


make_library(boost
  SOURCES
  ${boost_src_files}
  REQUIRES
  libbz2 z libjpeg libpng)


target_include_directories(boost SYSTEM PUBLIC ${__boost_src_dir})
target_include_directories(boost PRIVATE
  ${__boost_lib_dir}/chrono/stopwatches/include)


file(GLOB_RECURSE boost_test_src_files
  CONFIGURE_DEPENDS
  ${__boost_lib_dir}/test/src/*.cpp
)
list(FILTER boost_test_src_files EXCLUDE REGEX ".*/cpp_main.cpp")

make_library(boost_test
  SOURCES
  ${boost_test_src_files} 
  REQUIRES 
  boost)

target_compile_definitions(boost_test PUBLIC "-Dcpp_main=123")
