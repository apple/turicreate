#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <boost/iostreams/stream.hpp>
#include <core/storage/fileio/cache_stream_sink.hpp>
#include <core/storage/fileio/cache_stream_source.hpp>

using namespace turi::fileio;
using namespace turi::fileio_impl;


typedef boost::iostreams::stream<cache_stream_source> icache_stream;
typedef boost::iostreams::stream<cache_stream_sink> ocache_stream;

struct cache_stream_test {

 public:
   void test_read_write() {
     auto block = fixed_size_cache_manager::get_instance().new_cache("cache://0");

     std::string expected = "we require more minerals";
     ocache_stream out(block->get_cache_id());
     TS_ASSERT(out.good());
     out << expected;
     out.close();

     icache_stream in(block->get_cache_id());
     std::string value;
     TS_ASSERT(in.good());
     std::getline(in, value);
     TS_ASSERT(in.eof());
     in.close();
     TS_ASSERT_EQUALS(value, expected);
   }

   void test_read_write_large_blocks() {
     auto block = fixed_size_cache_manager::get_instance().new_cache("cache://1");

     ocache_stream out(block->get_cache_id());
     TS_ASSERT(out.good());

     const size_t BLOCK_SIZE = 1024; // 1K
     size_t NUM_BLOCKS = 1024; // 1K
     char buf[BLOCK_SIZE];
     for (size_t i = 0; i < NUM_BLOCKS; ++i) {
       memset(buf, (char)(i % 128), BLOCK_SIZE);
       out.write(buf, BLOCK_SIZE);
       TS_ASSERT(out.good());
     }
     out.close();

     icache_stream in(block->get_cache_id());
     TS_ASSERT(in.good());

     for (size_t i = 0; i < NUM_BLOCKS; ++i) {
       in.read(buf, BLOCK_SIZE);
       TS_ASSERT(in.good());
       for (size_t j = 0; j < BLOCK_SIZE; ++j) {
         TS_ASSERT_EQUALS(buf[j], (char)(i % 128));
       }
     }
     in.read(buf, 1);
     TS_ASSERT(in.eof());
     in.close();
   }

   void test_seek() {
     turi::fileio::FILEIO_MAXIMUM_CACHE_CAPACITY_PER_FILE = 1024*1024;
     const size_t block_size = turi::fileio::FILEIO_MAXIMUM_CACHE_CAPACITY_PER_FILE;
     _test_seek_helper(block_size / 2);
     _test_seek_helper(block_size);
     _test_seek_helper(block_size * 2);
   }

   /**
    * Write file_size bytes of data to the cache_stream, and test
    * read with random seek.
    */
   void _test_seek_helper(size_t file_size) {
     auto block = fixed_size_cache_manager::get_instance().new_cache("cache://2");

     ocache_stream out(block->get_cache_id());
     for (size_t i = 0; i < (file_size / sizeof(size_t)); ++i) {
       out.write(reinterpret_cast<char*>(&i), sizeof(size_t));
     }
     TS_ASSERT(out.good());
     out.close();

     icache_stream in(block->get_cache_id());
     for (size_t i = 0; i < (file_size / sizeof(size_t)); ++i) {
      size_t j = (i * 17) % (file_size / sizeof(size_t));
      in.seekg(j * sizeof(size_t), std::ios_base::beg);
      size_t v;
      in.read(reinterpret_cast<char*>(&v), sizeof(size_t));
      ASSERT_EQ(v, j);
    }
   }
};

BOOST_FIXTURE_TEST_SUITE(_cache_stream_test, cache_stream_test)
BOOST_AUTO_TEST_CASE(test_read_write) {
  cache_stream_test::test_read_write();
}
BOOST_AUTO_TEST_CASE(test_read_write_large_blocks) {
  cache_stream_test::test_read_write_large_blocks();
}
BOOST_AUTO_TEST_CASE(test_seek) {
  cache_stream_test::test_seek();
}
BOOST_AUTO_TEST_SUITE_END()
