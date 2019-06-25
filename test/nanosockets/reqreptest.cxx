/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <iostream>
#include <core/system/nanosockets/async_request_socket.hpp>
#include <core/system/nanosockets/async_reply_socket.hpp>
#include <core/system/nanosockets/publish_socket.hpp>
#include <core/system/nanosockets/subscribe_socket.hpp>
using namespace turi;
using namespace nanosockets; 

/**************************************************************************/
/*                                                                        */
/*                                 REPREQ                                 */
/*                                                                        */
/**************************************************************************/
size_t get_value(zmq_msg_vector& msgvec) {
  TS_ASSERT_EQUALS(msgvec.size(), 1);
  TS_ASSERT_EQUALS(msgvec.front()->length(), sizeof(size_t));
  iarchive iarc(msgvec.front()->data(), msgvec.front()->length());
  size_t val;
  iarc >> val;
  return val;
}

void set_value(zmq_msg_vector& msgvec, size_t val) {
  msgvec.clear();
  oarchive oarc;
  oarc << val;
  std::string s(oarc.buf, oarc.off);
  free(oarc.buf);
  msgvec.insert_back(s);
}
bool server_handler(zmq_msg_vector& recv,
                    zmq_msg_vector& reply) {
  size_t val = get_value(recv);
  set_value(reply, val + 1);
  return true;
}

volatile bool done = false;

void start_server(std::string address){  
  async_reply_socket reply(server_handler, 4, address);
  reply.start_polling();
  while (done == false) sleep(1);
}

void test_client(async_request_socket& sock, size_t id = 0) {
  for (size_t i = 0;i < 10000; ++i) {
    if (i % 1000 == 0) std::cout << id <<": " << i << std::endl;
    zmq_msg_vector req;
    zmq_msg_vector response;
    set_value(req, i);
    int rc = sock.request_master(req, response);
    TS_ASSERT_EQUALS(rc, 0);
    auto get_value_ret = get_value(response);
    TS_ASSERT_EQUALS(get_value_ret, i + 1);
  }
  std::cout << "Finished " << id << std::endl;
}

struct reqrep_test {
 public:
  void test_single_threaded() {
    done = false;
    std::string address = "inproc://aaa";
    thread_group grp;
    grp.launch([=](){start_server(address);});
    thread_group grp2;
    async_request_socket req(address);
    test_client(req);
    done = true;    
    grp.join();
  }
  void test_multi_thread() {
    done = false;
    std::string address = "inproc://bbb";
    thread_group grp;
    grp.launch([=](){start_server(address);});
    thread_group grp2;
    async_request_socket req(address);
    grp2.launch([&]() { test_client(req,0); });
    grp2.launch([&]() { test_client(req,1); });
    grp2.launch([&]() { test_client(req,2); });
    grp2.launch([&]() { test_client(req,3); });
    grp2.join();
    done = true;    
    grp.join();
  }
};

BOOST_FIXTURE_TEST_SUITE(_reqrep_test, reqrep_test)
BOOST_AUTO_TEST_CASE(test_single_threaded) {
  reqrep_test::test_single_threaded();
}
BOOST_AUTO_TEST_CASE(test_multi_thread) {
  reqrep_test::test_multi_thread();
}
BOOST_AUTO_TEST_SUITE_END()
