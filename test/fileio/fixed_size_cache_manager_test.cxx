#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <string>
#include <fileio/fixed_size_cache_manager.hpp>

using namespace turi::fileio;


struct fixed_size_cache_manager_test {

 public:

  cache_id_type make_cache_id(size_t i) {
    return cache_id_type("cache://" + std::to_string(i));
  }

  fixed_size_cache_manager_test() {
    // creates 10 new empty in-memory cache blocks.
    for (size_t i = 0; i < 10; ++i) {
      std::shared_ptr<cache_block> blk = fixed_size_cache_manager::get_instance().new_cache(make_cache_id(i));
      TS_ASSERT(blk->get_pointer() != NULL);
      TS_ASSERT(blk->get_pointer_size() == 0);
      TS_ASSERT(blk->get_filename() == "");
      TS_ASSERT(blk->get_pointer_capacity() == FILEIO_INITIAL_CAPACITY_PER_FILE);
    }
  }

  ~fixed_size_cache_manager_test() {
    fixed_size_cache_manager::get_instance().clear();
  }

  void test_new_cache() {
    // Write some data to the existing cache block.
    char dummy[30];
    std::fill(dummy, dummy + 30, 0);

    for (size_t i = 0; i < 10; ++i) {
      auto blk = fixed_size_cache_manager::get_instance().new_cache(make_cache_id(i));
      blk->write_bytes_to_memory_cache(dummy, 30);
    }
    for (size_t i = 0; i < 20; ++i) {
      auto blk = fixed_size_cache_manager::get_instance().new_cache(make_cache_id(i));
      TS_ASSERT(blk->get_pointer() != NULL);
      TS_ASSERT(blk->get_pointer_size() == 0);
      TS_ASSERT(blk->get_filename() == "");
      TS_ASSERT(blk->get_pointer_capacity() == FILEIO_INITIAL_CAPACITY_PER_FILE);
    }
  }

  void test_get_cache() {
    // Fetch each cache block and check member states.
    for (size_t i = 0; i < 10; ++i) {
      auto id = make_cache_id(i);
      auto blk = fixed_size_cache_manager::get_instance().get_cache(id);
      TS_ASSERT(blk->get_cache_id() == id);
      TS_ASSERT(blk->get_pointer() != NULL);
      TS_ASSERT(blk->get_pointer_size() == 0);
      TS_ASSERT(blk->get_filename() == "");
      TS_ASSERT(blk->get_pointer_capacity() == FILEIO_INITIAL_CAPACITY_PER_FILE);
      blk->write_bytes_to_memory_cache(reinterpret_cast<char*>(&i), sizeof(size_t));
    }
    // Throws exception when trying to get an invalid cache_id.
    TS_ASSERT_THROWS_ANYTHING(fixed_size_cache_manager::get_instance().get_cache(make_cache_id(11)));

    // Check the block data.
    for (size_t i = 0; i < 10; ++i) {
      auto blk = fixed_size_cache_manager::get_instance().get_cache(make_cache_id(i));
      TS_ASSERT(blk->get_pointer_size() == sizeof(size_t));
      size_t value = *((size_t*)blk->get_pointer());
      TS_ASSERT_EQUALS(value, i);
    }
  }

  void test_write_cache_to_file() {
    // Write the blocks to disk.
    for (size_t i = 0; i < 10; ++i) {
      auto blk = fixed_size_cache_manager::get_instance().get_cache(make_cache_id(i));
      blk->write_bytes_to_memory_cache(reinterpret_cast<char*>(&i), sizeof(size_t));
      blk->write_to_file();
    }

    for (size_t i = 0; i < 10; ++i) {
      auto blk = fixed_size_cache_manager::get_instance().get_cache(make_cache_id(i));
      TS_ASSERT(blk->get_pointer_size() == 0);
      TS_ASSERT(blk->get_pointer_capacity() == 0);
      TS_ASSERT(blk->get_pointer() == NULL);
      std::ifstream fin(blk->get_filename());
      char buf[256];
      fin.read(buf, sizeof(size_t)) ;
      fin.close();
      size_t value;
      memcpy(&value, buf, sizeof(size_t));
      TS_ASSERT_EQUALS(value, i);
    }
  }

  void test_free_cache() {
    for (size_t i = 0; i < 10; ++i) {
      auto blk = fixed_size_cache_manager::get_instance().get_cache(make_cache_id(i));
      fixed_size_cache_manager::get_instance().free(blk);
    }

    for (size_t i = 0; i < 10; ++i) {
      TS_ASSERT_THROWS_ANYTHING(fixed_size_cache_manager::get_instance().get_cache(make_cache_id(i)));
    }
  }

};



struct cache_eviction_test {
 public:
  void test_cache_eviction_mechanism() {
    // set cache cap to 64K
    auto& cache_instance = fixed_size_cache_manager::get_instance();
    turi::fileio::FILEIO_MAXIMUM_CACHE_CAPACITY = 64*1024;
    turi::fileio::FILEIO_MAXIMUM_CACHE_CAPACITY_PER_FILE = 32*1024;
    // now create a sequence of files ranging from 1K,2K,4K... 64K,128K,256K
    std::map<size_t, std::string> size_to_file;
    size_t fsize = 1024;
    while (fsize <= 256*1024) {
      std::string fname = cache_instance.get_temp_cache_id();
      logstream(LOG_INFO) << "Writing " << fname << " size = " << fsize << std::endl;
      turi::general_ofstream fout(fname);
      fout << std::string(fsize, 'A');
      size_to_file[fsize] = fname;
      fsize *= 2;
    }
    // when I write 64K, 32K should be evicted, and 64K will also eventually be evicted.
    // then when I write 128K, there is enough capacity to hold 1K -- 16K and to allocate 
    //        a new 32K block. So nothing else is evicted
    // similarly for 256K
    TS_ASSERT_EQUALS(cache_instance.get_cache(size_to_file[1*1024])->is_pointer(), true);
    TS_ASSERT_EQUALS(cache_instance.get_cache(size_to_file[2*1024])->is_pointer(), true);
    TS_ASSERT_EQUALS(cache_instance.get_cache(size_to_file[4*1024])->is_pointer(), true);
    TS_ASSERT_EQUALS(cache_instance.get_cache(size_to_file[8*1024])->is_pointer(), true);
    TS_ASSERT_EQUALS(cache_instance.get_cache(size_to_file[16*1024])->is_pointer(), true);
    TS_ASSERT_EQUALS(cache_instance.get_cache(size_to_file[32*1024])->is_pointer(), true);
    TS_ASSERT_EQUALS(cache_instance.get_cache(size_to_file[64*1024])->is_pointer(), false);
    TS_ASSERT_EQUALS(cache_instance.get_cache(size_to_file[128*1024])->is_pointer(), false);
    TS_ASSERT_EQUALS(cache_instance.get_cache(size_to_file[256*1024])->is_pointer(), false);
    // now to verify that things don't get evicted while it is still in size.
    // we are going to open the last one which is in memory (16K)
    // and set the cache block size to be large enough so that it will try to evict
    // something.
    turi::general_ifstream fin(size_to_file[16*1024]);
    turi::fileio::FILEIO_MAXIMUM_CACHE_CAPACITY_PER_FILE = 64*1024;
    // opening a new file should in theory evict this one, but since we are holding
    // a reference to it, it should not evict it, but should evict 2K
    std::string fname = cache_instance.get_temp_cache_id();
    turi::general_ofstream fout(fname);
    TS_ASSERT_EQUALS(cache_instance.get_cache(size_to_file[16*1024])->is_pointer(), true);
    TS_ASSERT_EQUALS(cache_instance.get_cache(size_to_file[8*1024])->is_pointer(), true);
    TS_ASSERT_EQUALS(cache_instance.get_cache(size_to_file[4*1024])->is_pointer(), true);
    TS_ASSERT_EQUALS(cache_instance.get_cache(size_to_file[2*1024])->is_pointer(), true);
    TS_ASSERT_EQUALS(cache_instance.get_cache(size_to_file[1*1024])->is_pointer(), true);
  }
};

BOOST_FIXTURE_TEST_SUITE(_fixed_size_cache_manager_test, fixed_size_cache_manager_test)
BOOST_AUTO_TEST_CASE(test_new_cache) {
  fixed_size_cache_manager_test::test_new_cache();
}
BOOST_AUTO_TEST_CASE(test_get_cache) {
  fixed_size_cache_manager_test::test_get_cache();
}
BOOST_AUTO_TEST_CASE(test_write_cache_to_file) {
  fixed_size_cache_manager_test::test_write_cache_to_file();
}
BOOST_AUTO_TEST_CASE(test_free_cache) {
  fixed_size_cache_manager_test::test_free_cache();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_cache_eviction_test, cache_eviction_test)
BOOST_AUTO_TEST_CASE(test_cache_eviction_mechanism) {
  cache_eviction_test::test_cache_eviction_mechanism();
}
BOOST_AUTO_TEST_SUITE_END()
