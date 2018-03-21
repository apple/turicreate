#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <string>
#include <stdio.h>
#include <fileio/general_fstream.hpp>
#include <fileio/hdfs.hpp>
#include <fileio/fs_utils.hpp>
#include <fileio/file_ownership_handle.hpp>
#include <fileio/file_handle_pool.hpp>
#include <fileio/fixed_size_cache_manager.hpp>
#include <logger/logger.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

struct general_fstream_test {

  char* tmpname;

  public:

    general_fstream_test() : 
#ifndef _WIN32
      tmpname (tmpnam(NULL)) {
#else
      tmpname( _tempnam(NULL, NULL) ) {
#endif
      global_logger().set_log_level(LOG_INFO);
    }

    void tearDownWorld() {
      fs::remove(fs::path(std::string(tmpname)));
    }

    void test_local_url() {
      std::string fname = std::string(tmpname);
      logstream(LOG_INFO) << "Test on url: " << fname  << std::endl;
      TS_ASSERT_EQUALS(helper_test_basic_read_write(fname), 0);
      TS_ASSERT_EQUALS(helper_test_seek(fname), 0);
    }

    void test_local_url_with_prefix() {
      std::string fname = "file://" + std::string(tmpname);
      logstream(LOG_INFO) << "Test on url: " << fname  << std::endl;
      TS_ASSERT_EQUALS(helper_test_basic_read_write(fname), 0);
      TS_ASSERT_EQUALS(helper_test_seek(fname), 0);
    }

    void test_caching_url() {
      std::string fname = "cache://" + std::string(tmpname);
      logstream(LOG_INFO) << "Test on url: " << fname  << std::endl;
      TS_ASSERT_EQUALS(helper_test_basic_read_write(fname), 0);
      TS_ASSERT_EQUALS(helper_test_seek(fname), 0);


      // fname = "cache://" + std::string(tmpname) + ".gz";
      fname = "./test.gz";
      logstream(LOG_INFO) << "Test on url: " << fname  << std::endl;
      TS_ASSERT_EQUALS(helper_test_basic_read_write(fname), 0);
      TS_ASSERT_THROWS_ANYTHING(helper_test_seek(fname));
    }

    void test_fs_util() {
      using namespace turi::fileio;
      TS_ASSERT_EQUALS(get_filename("/hello"), "hello");
      TS_ASSERT_EQUALS(get_filename("/hello/world.bin"), "world.bin");
      TS_ASSERT_EQUALS(get_filename("s3://world/pika.bin"), "pika.bin");
      TS_ASSERT_EQUALS(get_filename("file:///pika.bin"), "pika.bin");
      TS_ASSERT_EQUALS(get_filename("hdfs:///pika.bin"), "pika.bin");
      TS_ASSERT_EQUALS(get_filename("hdfs:///chu/pika.bin"), "pika.bin");
      TS_ASSERT_EQUALS(get_dirname("/hello"), "");
      TS_ASSERT_EQUALS(get_dirname("/hello/world.bin"), "/hello");
      TS_ASSERT_EQUALS(get_dirname("s3://world/pika.bin"), "s3://world");
      TS_ASSERT_EQUALS(get_dirname("hdfs:///pika.bin"), "hdfs://");
      TS_ASSERT_EQUALS(get_dirname("hdfs:///chu/pika.bin"), "hdfs:///chu");

      TS_ASSERT_EQUALS(make_absolute_path("/", "hello"), "/hello");
      TS_ASSERT_EQUALS(make_absolute_path("/pika", "hello"), "/pika/hello");
      TS_ASSERT_EQUALS(make_absolute_path("/pika/", "hello"), "/pika/hello");
      TS_ASSERT_EQUALS(make_absolute_path("file:///pika/", "hello"), "file:///pika/hello");
      TS_ASSERT_EQUALS(make_absolute_path("s3://pika/", "hello"), "s3://pika/hello");
      TS_ASSERT_EQUALS(make_absolute_path("hdfs:///pika/", "hello"), "hdfs:///pika/hello");
      TS_ASSERT_EQUALS(make_absolute_path("hdfs:///", "hello"), "hdfs:///hello");
      TS_ASSERT_EQUALS(make_absolute_path("hdfs://", "hello"), "hdfs:///hello");

      TS_ASSERT_EQUALS(make_relative_path("/", "/hello"), "hello");
      TS_ASSERT_EQUALS(make_relative_path("/pika", "/pika/hello"), "hello");
      TS_ASSERT_EQUALS(make_relative_path("/pika", "/pika2/hello"), "../pika2/hello");
      TS_ASSERT_EQUALS(make_relative_path("s3://pika/", "s3://pika/hello"), "hello");
      TS_ASSERT_EQUALS(make_relative_path("hdfs://pika/", "hdfs://pika/hello"), "hello");
      TS_ASSERT_EQUALS(make_relative_path("hdfs:///", "hdfs:///hello"), "hello");
      TS_ASSERT_EQUALS(make_relative_path("hdfs://", "hdfs:///hello"), "hello");
      TS_ASSERT_EQUALS(make_relative_path("/pika/hello/world", "/pika/fish/fillet"), "../../fish/fillet");
      TS_ASSERT_EQUALS(make_relative_path("/pika/hello/world", "/pika/hello/fillet"), "../fillet");
      TS_ASSERT_EQUALS(make_relative_path("/pika/hello", "/pika/hello/fillet"), "fillet");
      TS_ASSERT_EQUALS(make_relative_path("/pika/hello/world", "/pokemon/fish/fillet"), "../../../pokemon/fish/fillet");
      TS_ASSERT_EQUALS(make_relative_path("/pika/hello/world/", "/pokemon/fish/fillet"), "../../../pokemon/fish/fillet");

      TS_ASSERT_EQUALS(make_relative_path("hdfs:///pika/hello/world", "hdfs:///pika/fish/fillet"), "../../fish/fillet");
      TS_ASSERT_EQUALS(make_relative_path("hdfs:///pika/hello/world", "hdfs:///pika/hello/fillet"), "../fillet");
      TS_ASSERT_EQUALS(make_relative_path("hdfs:///pika/hello", "hdfs:///pika/hello/fillet"), "fillet");
      TS_ASSERT_EQUALS(make_relative_path("hdfs:///pika/hello/world", "hdfs:///pokemon/fish/fillet"), "../../../pokemon/fish/fillet");
      TS_ASSERT_EQUALS(make_relative_path("hdfs:///pika/hello/world/", "hdfs:///pokemon/fish/fillet"), "../../../pokemon/fish/fillet");

      TS_ASSERT_EQUALS(make_relative_path("s3:///pika/hello/world", "s3:///pika/fish/fillet"), "../../fish/fillet");
      TS_ASSERT_EQUALS(make_relative_path("s3:///pika/hello/world", "s3:///pika/hello/fillet"), "../fillet");
      TS_ASSERT_EQUALS(make_relative_path("s3:///pika/hello", "s3:///pika/hello/fillet"), "fillet");
      TS_ASSERT_EQUALS(make_relative_path("s3:///pika/hello/world", "s3:///pokemon/fish/fillet"), "../../../pokemon/fish/fillet");
      TS_ASSERT_EQUALS(make_relative_path("s3:///pika/hello/world/", "s3:///pokemon/fish/fillet"), "../../../pokemon/fish/fillet");

      TS_ASSERT_EQUALS(get_protocol("hdfs://"), "hdfs");
      TS_ASSERT_EQUALS(get_protocol("s3://pikachu"), "s3");
      TS_ASSERT_EQUALS(get_protocol("/pikachu"), "");
      TS_ASSERT_EQUALS(get_protocol("file:///pikachu"), "");
      TS_ASSERT_EQUALS(get_protocol("http://pikachu"), "http");

      TS_ASSERT_EQUALS(remove_protocol("hdfs://"), "");
      TS_ASSERT_EQUALS(remove_protocol("file://"), "");
      TS_ASSERT_EQUALS(remove_protocol("file://peekaboo"), "peekaboo");
      TS_ASSERT_EQUALS(remove_protocol("s3://pikachu"), "pikachu");
      TS_ASSERT_EQUALS(remove_protocol("/pikachu"), "/pikachu");
      TS_ASSERT_EQUALS(remove_protocol("http://pikachu://pikachu"), "pikachu://pikachu");
    }

    int helper_test_basic_read_write(const std::string& url) {
      std::string s;
      s.resize(16);
      for (size_t i = 0;i < 8; ++i) {
        s[2 * i] = 255;
        s[2 * i + 1] = 'a';
      }
      std::string expected;
      std::string buffer;
      try {
        std::cout << "Write to: " << url << std::endl;
        turi::general_ofstream fout(url);
        for (size_t i = 0;i < 4096; ++i) {
          fout.write(s.c_str(), s.length());
          expected = expected + s;
        }
        ASSERT_TRUE(fout.good());
        fout.close();

        std::cout << "Read from: " << url << std::endl;
        turi::general_ifstream fin(url);
        getline(fin, buffer);
        ASSERT_EQ(buffer, expected);
        fin.close();
      } catch(std::string& e) {
        std::cerr << "Exception: " << e << std::endl;
      }
      if (buffer != expected) return 1;
      return 0;
    }

    int helper_test_seek(const std::string& url) {
      std::cout << "Rewriting for seek test: " << url << std::endl;
      {
        turi::general_ofstream fout(url);
        for (size_t i = 0;i < 4096; ++i) {
          // write a 4K block
          fout.write(reinterpret_cast<char*>(&i), sizeof(size_t));
          char c[4096] = {0};
          fout.write(c, 4096 - sizeof(size_t));
        }
        ASSERT_TRUE(fout.good());
        fout.close();
      }
      std::cout << "Seeking everywhere in : " << url << std::endl;
      {
        turi::general_ifstream fin(url);
        for (size_t i = 0; i< 4096; ++i) {
          size_t j = (i * 17) % 4096;
          fin.seekg(4096 * j, std::ios_base::beg);
          size_t v;
          fin.read(reinterpret_cast<char*>(&v), sizeof(size_t));
          ASSERT_EQ(v, j);
        }
      }
      return 0;
    }

    void test_file_ownership_handle() {
      using namespace turi::fileio;
      std::string f = fixed_size_cache_manager::get_instance().get_temp_cache_id();
      fixed_size_cache_manager::get_instance().new_cache(f);
      {
        turi::fileio::file_ownership_handle h(f);
        // no throw
        fixed_size_cache_manager::get_instance().get_cache(f);
      }

      TS_ASSERT_THROWS_ANYTHING(fixed_size_cache_manager::get_instance().get_cache(f));
    }

    void test_file_handle_pool() {
      using namespace turi::fileio;

      auto& pool = turi::fileio::file_handle_pool::get_instance();

      delete_path(tmpname);
      TS_ASSERT_EQUALS((int)turi::fileio::get_file_status(tmpname), (int)turi::fileio::file_status::MISSING);

      {
        std::cout << "Write to: " << tmpname << std::endl;
        turi::general_ofstream fout(tmpname);
        std::string expected;
        std::string c = "abc";
        for (size_t i = 0;i < 4096; ++i) {
          fout.write(c.c_str(), c.length());
          expected = expected + "abc";
        }
        ASSERT_TRUE(fout.good());
        fout.close();

        auto handle = pool.register_file(tmpname);
        // on out of scope, the file should still exists
      }

      TS_ASSERT_EQUALS((int)turi::fileio::get_file_status(tmpname), (int)turi::fileio::file_status::REGULAR_FILE);

      {
        auto handle = pool.register_file(tmpname);

        // Now mark the file as deleted
        pool.mark_file_for_delete(tmpname);
      }

      // the file should be gone
      TS_ASSERT_EQUALS((int)turi::fileio::get_file_status(tmpname), (int)turi::fileio::file_status::MISSING);
    }
};

BOOST_FIXTURE_TEST_SUITE(_general_fstream_test, general_fstream_test)
BOOST_AUTO_TEST_CASE(test_local_url) {
  general_fstream_test::test_local_url();
}
BOOST_AUTO_TEST_CASE(test_caching_url) {
  general_fstream_test::test_caching_url();
}
BOOST_AUTO_TEST_CASE(test_fs_util) {
  general_fstream_test::test_fs_util();
}
BOOST_AUTO_TEST_CASE(test_file_ownership_handle) {
  general_fstream_test::test_file_ownership_handle();
}
BOOST_AUTO_TEST_CASE(test_file_handle_pool) {
  general_fstream_test::test_file_handle_pool();
}
BOOST_AUTO_TEST_SUITE_END()
