#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <iostream>
#include <core/system/lambda/worker_pool.hpp>
#include <core/parallel/lambda_omp.hpp>
#include <core/storage/fileio/fs_utils.hpp>
#include <core/system/nanosockets/socket_config.hpp>
#include "dummy_worker_interface.hpp"

using namespace turi;

struct worker_pool_test {
 public:
  worker_pool_test() {
    global_logger().set_log_level(LOG_INFO);

    // Manually set this one.
    char* use_fallback = std::getenv("TURI_FORCE_IPC_TO_TCP_FALLBACK");
    if(use_fallback != nullptr && std::string(use_fallback) == "1") {
      nanosockets::FORCE_IPC_TO_TCP_FALLBACK = true;
    }
  }
  void test_spawn_workers() {
    auto wk_pool = get_worker_pool(nworkers);
    TS_ASSERT_EQUALS(wk_pool->num_workers(), nworkers);
    TS_ASSERT_EQUALS(wk_pool->num_available_workers(), nworkers);
  }

  void test_get_and_release_worker() {
    auto wk_pool = get_worker_pool(nworkers);
    parallel_for(0, (size_t)16, [&](size_t i) {
      std::string message = std::to_string(i);
      auto worker = wk_pool->get_worker();
      TS_ASSERT(worker->proxy->echo(message).compare(message) == 0);
      wk_pool->release_worker(worker);
    });
  }

  void test_worker_guard() {
    auto wk_pool = get_worker_pool(nworkers);
    parallel_for(0, nworkers * 4, [&](size_t i) {
      std::string message = std::to_string(i);
      auto worker = wk_pool->get_worker();
      auto guard = wk_pool->get_worker_guard(worker);
      TS_ASSERT(worker->proxy->echo(message).compare(message) == 0);
      TS_ASSERT_THROWS_ANYTHING(worker->proxy->throw_error());
    });
  }

  void test_worker_crash_and_restart() {
    auto wk_pool = get_worker_pool(nworkers);
    {
      auto worker = wk_pool->get_worker();
      auto guard = wk_pool->get_worker_guard(worker);
      TS_ASSERT_THROWS(worker->proxy->quit(0), cppipc::ipcexception);
    }
    TS_ASSERT_EQUALS(wk_pool->num_workers(), nworkers);

    parallel_for(0, nworkers, [&](size_t i) {
      std::string message = std::to_string(i);
      auto worker = wk_pool->get_worker();
      auto guard = wk_pool->get_worker_guard(worker);
      TS_ASSERT(worker->proxy->echo(message).compare(message) == 0);
      TS_ASSERT_THROWS(worker->proxy->quit(0), cppipc::ipcexception);
    });

    TS_ASSERT_EQUALS(wk_pool->num_workers(), nworkers);
    TS_ASSERT_EQUALS(wk_pool->num_available_workers(), nworkers);

    parallel_for(0, nworkers, [&](size_t i) {
       std::string message = std::to_string(i);
       auto worker = wk_pool->get_worker();
       auto guard = wk_pool->get_worker_guard(worker);
       TS_ASSERT(worker->proxy->echo(message).compare(message) == 0);
     });
  }

  void test_call_all_workers() {
    auto wk_pool = get_worker_pool(nworkers);
    auto f = [](std::unique_ptr<dummy_worker_proxy>& proxy) {
      proxy->echo("");
      return 0;
    };
    auto ret = wk_pool->call_all_workers<int>(f);
    TS_ASSERT_EQUALS(ret.size(), nworkers);
  }

  void test_call_all_workers_with_exception() {
    auto wk_pool = get_worker_pool(nworkers);
    auto f = [](std::unique_ptr<dummy_worker_proxy>& proxy) {
      proxy->throw_error();
      return 0;
    };
    TS_ASSERT_THROWS_ANYTHING(wk_pool->call_all_workers<int>(f));
    TS_ASSERT_EQUALS(wk_pool->num_workers(), nworkers);
    TS_ASSERT_EQUALS(wk_pool->num_available_workers(), nworkers);
  }

  void test_call_all_workers_with_crash_recovery() {
    auto wk_pool = get_worker_pool(nworkers);
    auto bad_fun = [](std::unique_ptr<dummy_worker_proxy>& proxy) {
      proxy->quit(0);
      return 0;
    };
    TS_ASSERT_THROWS(wk_pool->call_all_workers<int>(bad_fun), cppipc::ipcexception);

    // call_all_worker should recover after crash
    TS_ASSERT_EQUALS(wk_pool->num_workers(), nworkers);
    TS_ASSERT_EQUALS(wk_pool->num_available_workers(), nworkers);

    auto good_fun = [](std::unique_ptr<dummy_worker_proxy>& proxy) {
      proxy->echo("");
      return 0;
    };
    TS_ASSERT_EQUALS(wk_pool->call_all_workers<int>(good_fun).size(), nworkers);
  }

  std::shared_ptr<lambda::worker_pool<dummy_worker_proxy>> get_worker_pool(size_t poolsize) {
    int timeout = 1;
    std::shared_ptr<lambda::worker_pool<dummy_worker_proxy>> ret;
    ret.reset(new lambda::worker_pool<dummy_worker_proxy>(poolsize, {worker_binary}, timeout));
    return ret;
  };

  size_t nworkers = 3;
#ifndef _WIN32
  const std::string worker_binary = "./dummy_worker";
#else
  const std::string worker_binary = "./dummy_worker.exe";
#endif
};

BOOST_FIXTURE_TEST_SUITE(_worker_pool_test, worker_pool_test)
BOOST_AUTO_TEST_CASE(test_spawn_workers) {
  worker_pool_test::test_spawn_workers();
}
BOOST_AUTO_TEST_CASE(test_get_and_release_worker) {
  worker_pool_test::test_get_and_release_worker();
}
BOOST_AUTO_TEST_CASE(test_worker_guard) {
  worker_pool_test::test_worker_guard();
}
BOOST_AUTO_TEST_CASE(test_worker_crash_and_restart) {
  worker_pool_test::test_worker_crash_and_restart();
}
BOOST_AUTO_TEST_CASE(test_call_all_workers) {
  worker_pool_test::test_call_all_workers();
}
BOOST_AUTO_TEST_CASE(test_call_all_workers_with_exception) {
  worker_pool_test::test_call_all_workers_with_exception();
}
BOOST_AUTO_TEST_CASE(test_call_all_workers_with_crash_recovery) {
  worker_pool_test::test_call_all_workers_with_crash_recovery();
}
BOOST_AUTO_TEST_SUITE_END()
