#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/system/cppipc/cppipc.hpp>
#include <core/system/cppipc/common/authentication_token_method.hpp>
#include <core/storage/fileio/temp_files.hpp>
#include <thread>
#include <chrono>
#include "test_object_base.hpp"

void pester_server_with_new_friends(cppipc::comm_client& client, size_t num_times) {
  for(size_t i = 0; i < num_times; ++i) {
    try {
      test_object_proxy test_object(client);
      std::cout << test_object.ping("hello world") << "\n";
    } catch (cppipc::reply_status status) {
      std::cout << "Exception: " << cppipc::reply_status_to_string(status) << "\n";
    } catch (const char* s) {
      std::cout << "Exception: " << s << "\n";
    }
  }
}

struct garbage_collect_test  {
  public:
    void test_gc_session() {
      //boost::thread server(server_thread_func);
      // Start server
      std::string prefix = turi::get_temp_name();
      std::string server_ipc_file = std::string("ipc://"+prefix);
      cppipc::comm_server server({}, "", server_ipc_file);

      server.register_type<test_object_base>([](){ return new test_object_impl;});

      std::cout << "Server gonna start now!" << std::endl;
      server.start();
      // Start client
      cppipc::comm_client client({}, server_ipc_file);

      client.start();

      // We start with one object tracked
      TS_ASSERT_EQUALS(server.num_registered_objects(), 1); // client

      pester_server_with_new_friends(client, 14);

      std::this_thread::sleep_for(std::chrono::seconds(2)); 
      test_object_proxy thing(client);
      TS_ASSERT_EQUALS(server.num_registered_objects(), 2); // client and thing

      std::shared_ptr<test_object_proxy> thing2 = std::make_shared<test_object_proxy>(client);

      // Test to see if server-created objects are created and deleted correctly
      std::shared_ptr<test_object_proxy> p = 
          std::dynamic_pointer_cast<test_object_proxy>(thing - thing2);
      ASSERT_NE(p.get(), NULL);
      TS_ASSERT_EQUALS(server.num_registered_objects(), 4); // client, thing, thing2, and p

      // Execute a function that returns an existing object
      std::shared_ptr<test_object_proxy> q = std::dynamic_pointer_cast<test_object_proxy>(thing + thing2);
      ASSERT_NE(q.get(), NULL);
      TS_ASSERT_EQUALS(server.num_registered_objects(), 4);

      // Sync objects on delete
      std::this_thread::sleep_for(std::chrono::seconds(2));
      p.reset();
      q.reset();
      TS_ASSERT_EQUALS(server.num_registered_objects(), 3); // client, thing, thing2

      // Failed client and reconnect
      client.stop();

      cppipc::comm_client next_client({}, server_ipc_file); 

      // New client
      next_client.start();
      TS_ASSERT_EQUALS(server.num_registered_objects(), 3); // new client, thing, thing2
      test_object_proxy new_thing(next_client);
      std::shared_ptr<test_object_proxy> new_thing2 = std::make_shared<test_object_proxy>(next_client);

      next_client.stop();
    }
};

BOOST_FIXTURE_TEST_SUITE(_garbage_collect_test, garbage_collect_test)
BOOST_AUTO_TEST_CASE(test_gc_session) {
  garbage_collect_test::test_gc_session();
}
BOOST_AUTO_TEST_SUITE_END()
