/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/system/cppipc/cppipc.hpp>
#include <core/system/cppipc/common/object_factory_base.hpp>
#include "test_object_base.hpp"

int main(int argc, char** argv) {
//   cppipc::comm_client client({"localhost:2181"}, "test");
  cppipc::comm_client client({}, "tcp://127.0.0.1:19000");
  /*
  cppipc::comm_client client({}, 
                             "ipc:///tmp/cppipc_server_test");
                             */
  client.start();
  client.add_status_watch(WATCH_COMM_SERVER_INFO, 
                          [](std::string message) {
                          std::cout << message << "\n";
                          });

  for (size_t i = 0;i < 100; ++i) {
  try { 
    test_object_proxy test_object(client);
    std::cout << test_object.ping("hello world") << "\n";

    std::cout << "5 + 1 = " << test_object.add_one(5, "hello") << "\n";
    std::cout << "5 + 5 = " << test_object.add(5, 5) << "\n";
    std::cout << "5 - 5 = " << test_object.subtract(5, 5) << "\n";
    std::cout << "return_one = " << test_object.return_one() << "\n";
    ASSERT_EQ(test_object.add(5, 5), 10);
    ASSERT_EQ(test_object.subtract(5, 5), 0);
  } catch (cppipc::reply_status status) {
    std::cout << "Exception: " << cppipc::reply_status_to_string(status) << "\n";
  } catch (const char* s) {
    std::cout << "Exception: " << s << "\n";
  }
  }

  std::shared_ptr<test_object_proxy> pika(std::make_shared<test_object_proxy>(client));
  std::shared_ptr<test_object_proxy> chu(std::make_shared<test_object_proxy>(client));

  pika->set_value(10);
  chu->set_value(5);
  ASSERT_EQ(pika->get_value(), 10);
  ASSERT_EQ(chu->get_value(), 5);

  pika->subtract_from(std::static_pointer_cast<test_object_base>(chu));
  ASSERT_EQ(pika->get_value(), 5);
  ASSERT_EQ(chu->get_value(), 5);

  chu->subtract_from(std::static_pointer_cast<test_object_base>(pika));
  ASSERT_EQ(pika->get_value(), 5);
  ASSERT_EQ(chu->get_value(), 0);

  pika->swap(std::static_pointer_cast<test_object_base>(chu));
  ASSERT_EQ(pika->get_value(), 0);
  ASSERT_EQ(chu->get_value(), 5);

  chu->swap(std::static_pointer_cast<test_object_base>(pika));
  ASSERT_EQ(pika->get_value(), 5);
  ASSERT_EQ(chu->get_value(), 0);

  chu->set_value(2);
  std::shared_ptr<test_object_proxy> p = std::dynamic_pointer_cast<test_object_proxy>(*pika - chu);
  ASSERT_NE(p.get(), NULL);
  ASSERT_EQ(p->get_value(), 3);

  // Test objects with reference count greater than 1
  std::shared_ptr<test_object_proxy> q = std::dynamic_pointer_cast<test_object_proxy>(*pika + chu);
  ASSERT_EQ(q->get_value(), 7);
  ASSERT_EQ(chu->get_value(), 7);
  ASSERT_EQ(pika->get_value(), 5);

  bool exception_caught = false;
  try {
    chu->an_exception();
  } catch (cppipc::ipcexception& except) {
    std::cout << except.what() << "\n";
    exception_caught = true;
  }

  ASSERT_TRUE(exception_caught);

  // ping test with increasing lengths
  test_object_proxy test_object(client);
  for (size_t i = 0;i <= 25; ++i) {
    size_t j = ((size_t)1 << i) - 1;
    turi::timer ti;
    std::cout << "Sending ping of length " << j << std::endl;
    std::string ret = test_object.return_big_object(j);
    std::cout << "Ping of length " << j << " RTT = " << ti.current_time() << "s" << std::endl;
    ASSERT_EQ(ret.length(), j);
  }
  p.reset();
}
