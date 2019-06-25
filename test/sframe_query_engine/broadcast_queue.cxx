#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/query_engine/util/broadcast_queue.hpp>

using namespace turi;

struct broadcast_queue_test {
 public:
   void test_broadcast_queue_one_consumer() {
     // 1 consumer, 4 cache limit
     constexpr size_t BUF_LIMIT = 4;
     broadcast_queue<size_t> bq(1, BUF_LIMIT);
     size_t readval = 0;
     size_t writeval = 0;
     size_t val = 0;
     for (size_t i = 1; i < 32; ++i) {
       for (size_t j = 0; j < i; ++j) {
         bq.push(writeval++);
       }
       TS_ASSERT(!bq.empty(0));
       if (i > 2 * BUF_LIMIT) {
         auto util = fileio::fixed_size_cache_manager::get_instance().get_cache_utilization();
         TS_ASSERT(util > 0);
       }
       for (size_t j = 0; j < i; ++j) {
         TS_ASSERT_EQUALS(bq.pop(0, val), true);
         TS_ASSERT_EQUALS(val, readval);
         ++readval;
       }
       TS_ASSERT_EQUALS(bq.pop(0, val), false);
       TS_ASSERT(bq.empty(0));
       // all buffers should be cleared. utlization should be 0
       bq.delete_all_cache_files();
       auto util = fileio::fixed_size_cache_manager::get_instance().get_cache_utilization();
       TS_ASSERT_EQUALS(util, 0);
     }
   }
   void test_broadcast_queue_k_consumer() {
     // 1 consumer, 4 cache limit
     constexpr size_t K = 4;
     constexpr size_t BUF_LIMIT = 4;
     broadcast_queue<size_t> bq(K, BUF_LIMIT);
     size_t readval[K] = {0};
     size_t writeval = 0;
     size_t val = 0;
     for (size_t i = 1; i < 32; ++i) {
       for (size_t j = 0; j < i; ++j) {
         bq.push(writeval++);
       }
       if (i > 2 * BUF_LIMIT) {
         auto util = fileio::fixed_size_cache_manager::get_instance().get_cache_utilization();
         TS_ASSERT(util > 0);
       }
       for (size_t k = 0; k < K; ++k) {
         TS_ASSERT(!bq.empty(k));
       }
       for (size_t k = 0; k < K; ++k) {
         for (size_t j = 0; j < i; ++j) {
           TS_ASSERT_EQUALS(bq.pop(k, val), true);
           TS_ASSERT_EQUALS(val, readval[k]);
           ++readval[k];
         }
         TS_ASSERT_EQUALS(bq.pop(k, val), false);
         TS_ASSERT(bq.empty(k));
       }
       // all buffers should be cleared. utlization should be 0
       bq.delete_all_cache_files();
       auto util = fileio::fixed_size_cache_manager::get_instance().get_cache_utilization();
       TS_ASSERT_EQUALS(util, 0);
     }
   }

   void test_broadcast_queue_k_consumer_variable_scheduling() {
     // 1 consumer, 4 cache limit
     // read and write in arbitrary order
     constexpr size_t K = 20;
     constexpr size_t BUF_LIMIT = 30;
     broadcast_queue<size_t> bq(K, BUF_LIMIT);
     size_t readval[K] = {0};
     size_t writeval = 0;
     size_t val = 0;
     size_t max_writeval = 10000;
     auto seed = time(NULL);
     std::cout << "Seed: " << seed << "\n";
     srand(seed); 
     while(1) {
       size_t n_to_push = rand() % 100;
       n_to_push = std::min(n_to_push, max_writeval - writeval);
       for (size_t i = 0;i < n_to_push; ++i) bq.push(writeval++);

       for (size_t k = 0; k < K; ++k) {
         size_t n_to_read = rand() % 100;
         n_to_read = std::min(n_to_read, writeval - readval[k]);
         if (writeval > readval[k]) {
           TS_ASSERT(!bq.empty(k));
         }
         for (size_t j = 0; j < n_to_read; ++j) {
           TS_ASSERT_EQUALS(bq.pop(k, val), true);
           if (val != readval[k]) {
             TS_ASSERT_EQUALS(val, readval[k]);
             std::cout << "!" << std::endl;
           }
           ++readval[k];
         }
         if (readval[k] == writeval) {
           TS_ASSERT(bq.empty(k));
           TS_ASSERT_EQUALS(bq.pop(k, val), false);
         }
       }

       bool all_readers_done = std::all_of(readval, readval + K,
                                           [&](size_t i) {
                                             return i == max_writeval;
                                           });
       if (writeval == max_writeval && all_readers_done) break;
     }
     // all buffers should be cleared. utlization should be 0
     bq.delete_all_cache_files();
     auto util = fileio::fixed_size_cache_manager::get_instance().get_cache_utilization();
     TS_ASSERT_EQUALS(util, 0);
   }
};


BOOST_FIXTURE_TEST_SUITE(_broadcast_queue_test, broadcast_queue_test)
BOOST_AUTO_TEST_CASE(test_broadcast_queue_one_consumer) {
  broadcast_queue_test::test_broadcast_queue_one_consumer();
}
BOOST_AUTO_TEST_CASE(test_broadcast_queue_k_consumer) {
  broadcast_queue_test::test_broadcast_queue_k_consumer();
}
BOOST_AUTO_TEST_CASE(test_broadcast_queue_k_consumer_variable_scheduling) {
  broadcast_queue_test::test_broadcast_queue_k_consumer_variable_scheduling();
}
BOOST_AUTO_TEST_SUITE_END()
