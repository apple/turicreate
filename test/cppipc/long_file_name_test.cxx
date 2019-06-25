#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/system/cppipc/cppipc.hpp>
#include <core/system/cppipc/common/authentication_token_method.hpp>
#include <core/storage/fileio/temp_files.hpp>
#include <thread>
#include <chrono>
#include "test_object_base.hpp"

struct test_long_file_name {
  public:
    void test_lfn() {
      //boost::thread server(server_thread_func);
      // Start server
      std::string prefix = turi::get_temp_name();
      // make prefix more than 130 bytes long. Just append a bunch of 'a's
      if (prefix.length() < 130) prefix += std::string(130 - prefix.length(), 'a');
      std::cout << "Target address: " << "ipc://" << prefix << "\n";
      std::string server_ipc_file = std::string("ipc://"+prefix);
      cppipc::comm_server server({}, "", server_ipc_file);

      server.register_type<test_object_base>([](){ return new test_object_impl;});

      std::cout << "Server gonna start now!" << std::endl;
      server.start();
      // Start client
      cppipc::comm_client client({}, server_ipc_file);

      client.start();
      {
        test_object_proxy test_object(client);
        TS_ASSERT_EQUALS(test_object.ping("hello world"), "hello world");
      }
      client.stop();
    }
};

BOOST_FIXTURE_TEST_SUITE(_test_long_file_name, test_long_file_name)
BOOST_AUTO_TEST_CASE(test_lfn) {
  test_long_file_name::test_lfn();
}
BOOST_AUTO_TEST_SUITE_END()
