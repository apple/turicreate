#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <string>
#include <iostream>
#include <shmipc/shmipc.hpp>
#include <parallel/pthread_tools.hpp>
using namespace turi;
// 16 byte buffer 
const size_t BUFFER_SIZE = 16;
struct shmipc_test {
 public:
  shmipc::server server;
  shmipc::client client;
  std::string server_address;

  shmipc::server large_server;
  shmipc::client large_client;
  std::string large_server_address;


  void server_process() {
    TS_ASSERT(server.wait_for_connect(60));
    // tis is just a pinging server
    while(1) {
      char *c = nullptr;
      size_t len = 0;
      size_t receivelen = 0;
      bool ok = server.receive_direct(&c, &len, receivelen, 10);
      if (ok == false) {
        // we took longer than 10 seconds? consider this a failure and quit
        TS_FAIL("SHMIPC Server timeout waiting for client");
        break;
      }
      if (receivelen >= 3) {
        if (strncmp(c, "end", 3) == 0) break;
      } 
      TS_ASSERT(server.send(c, receivelen));
    }
    server.shutdown();
  }

  void client_process() {
    TS_ASSERT(client.connect(server_address, 60));

    std::vector<std::string> messages{"hello", "world"};
    for (auto message: messages) {
      TS_ASSERT(client.send(message.c_str(), message.length()));
      char *c = nullptr;
      size_t len = 0;  
      size_t receivelen = 0;
      TS_ASSERT(client.receive_direct(&c, &len, receivelen, 10));
      std::string s(c, receivelen);
      TS_ASSERT_EQUALS(s, message);
    }

    // too large buffer
    std::string bigger_than_buffer(BUFFER_SIZE + 1, 'a');
    TS_ASSERT_EQUALS(client.send(bigger_than_buffer.c_str(), BUFFER_SIZE + 1), 
                     false);
    // send the termination call
    client.send("end", 3);
  }

  void test_connect() {
    TS_ASSERT(server.bind("", BUFFER_SIZE));
    server_address = server.get_shared_memory_name();
    
    thread_group group;
    group.launch([=](){ this->server_process();});
    group.launch([=](){ this->client_process();});
    group.join();
  }

  void large_server_process() {
    TS_ASSERT(large_server.wait_for_connect(60));
    // tis is just a pinging server
    while(1) {
      char *c = nullptr;
      size_t len = 0;
      size_t receivelen = 0;
      bool ok = large_receive(large_server, &c, &len, receivelen, 10);
      if (ok == false) {
        // we took longer than 10 seconds? consider this a failure and quit
        TS_FAIL("SHMIPC Server timeout waiting for client");
        break;
      }
      if (receivelen >= 3) {
        if (strncmp(c, "end", 3) == 0) break;
      } 
      TS_ASSERT(large_send(large_server, c, receivelen));
      free(c);
    }
    large_server.shutdown();
  }

  void large_client_process() {
    TS_ASSERT(large_client.connect(large_server_address, 60));

    std::vector<std::string> messages{"hello", "world", "bighello", "bigworld"};
    // rather large messages cat against self afew times
    for (size_t i = 0;i < 5; ++i) {
      messages[2] = messages[2] + messages[2];
      messages[3] = messages[3] + messages[3];
    }
    // this makes messages[3] not evenly divisible by 16
    messages[3] = messages[3] + "abc";
    for (auto message: messages) {
      TS_ASSERT(large_send(large_client, message.c_str(),message.length()));
      char *c = nullptr;
      size_t len = 0;  
      size_t receivelen = 0;
      TS_ASSERT(large_receive(large_client, &c, &len, receivelen, 10));
      std::string s(c, receivelen);
      TS_ASSERT_EQUALS(s, message);
      free(c);
    }
    // send the termination call
    large_client.send("end", 3);
  }


  void test_large_comm() {
    TS_ASSERT(large_server.bind("", BUFFER_SIZE));
    large_server_address = large_server.get_shared_memory_name();
    
    thread_group group;
    group.launch([=](){ this->large_server_process();});
    group.launch([=](){ this->large_client_process();});
    group.join();
  }
};


BOOST_FIXTURE_TEST_SUITE(_shmipc_test, shmipc_test)
BOOST_AUTO_TEST_CASE(test_connect) {
  shmipc_test::test_connect();
}
BOOST_AUTO_TEST_CASE(test_large_comm) {
  shmipc_test::test_large_comm();
}
BOOST_AUTO_TEST_SUITE_END()
