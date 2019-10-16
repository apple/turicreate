/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#include <atomic>
#include <core/storage/sframe_data/groupby_aggregate.hpp>
#include <core/storage/sframe_data/groupby_aggregate_operators.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/sframe_constants.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>
#include <iostream>
#include <random>
#include <thread>

namespace turi {
namespace {

/* global variables */
size_t thread_size_beg = 2;
bool sframe_debug_print = false;

// ---------------------- benchmark test framework -------------------------

/*
 *
 * It can only test one operator at one time. In this way,
 * ouput name is unique and not required from client since it's disposable.
 *
 * You only need to define two functions, one for defining
 * the operator run, and one for collecting the summary.
 *
 * sample usage:
 * \ref bench_test_aggreate_count_fn
 * \ref bench_test_aggreate_summary_count_bin
 *
 * \param sf: input sframe
 * \param reps: repeatitions
 * \param Runner: void (*)(const sframe& sf, size_t nthreads, size_t reps,
 *                         const std::vector<std::string>& keys,
 *                         const std::vector<std::string>& op_keys);
 *
 * \param keys: compsite keys to group by
 * \param op_keys: columns to operate on
 */
template <typename Runner>
void _bench_test_aggreate_runner(const sframe& sf, size_t reps, Runner runner,
                                 const std::vector<std::string>& keys,
                                 const std::vector<std::string>& op_keys) {
  size_t max_hardware_mp = turi::thread::cpu_count();

  for (size_t ii = thread_size_beg; ii < max_hardware_mp; ii *= 2) {
    runner(sf, ii, reps, keys, op_keys);
  }

  runner(sf, max_hardware_mp, reps, keys, op_keys);
}


/*
 * helper function to run aggregate with controlled thread pool size.
 *
 * \param nthreads: thread pool size
 * \param in_sf: input sframe
 * \param op: group-by aggregation operator
 * \param keys: compsite keys to group by
 * \param op_keys: columns to operate on
 *
 * */
sframe _bench_test_aggreate_with_pool(size_t nthreads, const sframe& in_sf,
                                    std::shared_ptr<group_aggregate_value> op,
                                    const std::vector<std::string>& keys,
                                    const std::vector<std::string>& op_keys,
                                    bool debug_print = sframe_debug_print) {
  ASSERT_MSG(nthreads > 0 && nthreads <= thread::cpu_count(),
             "invalid thread count");

  // set up groupby_aggregate_impl
  auto& pool = thread_pool::get_instance();
  size_t old_pl_sz = pool.size();

  bool need_to_set_thread_size = nthreads != pool.size();
  if (need_to_set_thread_size) {
    pool.resize(nthreads);
  }

  logstream(LOG_INFO) << "Bench test groupby with count with " << nthreads
                      << "threads." << std::endl;

  turi::timer ti;
  sframe out_sf = groupby_aggregate(
      in_sf, keys, {"__output_name_is_not_important"}, {{op_keys, op}});

  logstream(LOG_INFO) << "Bench test groupby with count finished in "
                      << ti.current_time() << std::endl;

  // restore the default thread pool size
  if (need_to_set_thread_size) pool.resize(old_pl_sz);

  if (debug_print) out_sf.debug_print();

  return out_sf;
}


// --------------------------- data generation helper functions -----------------------------


/*
 * generate sframe data of type T, whose value ranges
 * from [start, end) obeying a uniform distribution.
 *
 * \param nrows: number of rows.
 * \param start: the minimum value, inclusive.
 * \param end: the maximum value, exclusive.
 * \param keys: column names.
 *
 * */
template <typename T>
sframe _generate_range_data(size_t nrows, int start, int end,
                            const std::vector<std::string> keys) {
  ASSERT_MSG(start < end, "start should be less than end");
  ASSERT_MSG(keys.size(), "at least one key is required");

  std::default_random_engine gen;
  std::uniform_int_distribution<int> dist(start, end);
  size_t ncols = keys.size();

  // muttable due to the change of internal state of random engine.
  auto range_seq_gen = [=](size_t ii) mutable -> std::vector<flexible_type> {
    std::vector<flexible_type> ret;
    ret.reserve(ncols);
    for (size_t ii = 0; ii < ncols; ++ii) {
      ret.emplace_back(static_cast<T>(dist(gen)));
    }
    return ret;
  };

  sframe sf = make_testing_sframe(keys, {flexible_type(T()).get_type()}, nrows,
                                  range_seq_gen);
  return sf;
}


/*
 * generate sframe data of type T, whose value ranges
 * from [start, end) with a customized histogram distribution.
 *
 * this data generator is intended to benchmark the multithreading
 * performance of aggregation when there are heavy contentions caused
 * by majority samples.
 *
 * \param nrows: number of rows.
 * \param start: the minimum value, inclusive.
 * \param end: the maximum value, exclusive.
 * \param keys: column names.
 * \param percentages: <percentage of a value : value>, e.g.,
 * <10, 15> means value 15 will have 10% probability to show up
 * in results. The rest 90% will generate from range [start, end).
 * if value 15 is in range [start, end), then it may have over 10%
 * probability to show up in final results.
 *
 * */
template <typename T>
sframe _generate_weighted_data(size_t nrows, T start, T end,
                               const std::vector<std::string>& keys,
                               const std::map<size_t, T>& percentages) {
  ASSERT_MSG(keys.size(), "at least one key is required");
  ASSERT_MSG(start < end, "start should be less than end");

  std::vector<size_t> samples;
  std::vector<T> values;
  samples.reserve(percentages.size());

  {
    std::set<T> val_exclue;
    for (const auto& entry : percentages) {
      ASSERT_MSG(entry.second >= start && entry.second < end,
                "value not in range [start, end)");
      ASSERT_MSG(val_exclue.count(entry.second) == 0,
                "no duplicate value is allowed in 'percentages'");
      val_exclue.insert(entry.second);

      if (samples.empty()) {
        samples.emplace_back(entry.first);
        values.emplace_back(entry.second);
      } else {
        samples.emplace_back(samples.back() + entry.first);
        values.emplace_back(entry.second);
      }
    }
  }

  if (samples.size())
    ASSERT_MSG(samples.back() <= 100,
               "the sum of percentages should be less than 100");

  std::default_random_engine gen;
  std::uniform_int_distribution<int> dist_sam(0, 99);
  std::uniform_int_distribution<T> dist_dat(start, end);

  size_t ncols = keys.size();
  // muttable due to the change of internal state of random engine.
  auto range_seq_gen = [=](size_t ii) mutable -> std::vector<flexible_type> {
    std::vector<flexible_type> ret;
    ret.reserve(ncols);

    for (size_t ii = 0; ii < ncols; ++ii) {
      int sample = dist_sam(gen);
      auto itr = std::upper_bound(samples.begin(), samples.end(), sample);

      if (itr != samples.end()) {
        ret.emplace_back(values[itr - samples.begin()]);
      } else {
        ret.emplace_back(dist_dat(gen));
      }
    }

    return ret;
  };

  sframe sf = make_testing_sframe(keys, {flexible_type(T()).get_type()}, nrows,
                                  range_seq_gen);
  return sf;
}


/*
 * Testing min operation.
 *
 * it should initialize the aggregation operator for _bench_test_aggreate_with_pool
 * to run with.
 *
 * It's client's responsibility to make sure key and op_keys correct
 *
 * \param sf: input sframe
 * \param nthreads: thread pool control
 * \param op: group-by aggregation operator
 * \param keys: compsite keys to group by
 * \param op_keys: columns to operate on
 * */
#define DEFINE_RUNNER(op_name, op)                                       \
  void bench_test_aggreate_fn_##op_name(                                 \
      const sframe& sf, size_t nthreads, size_t reps,                    \
      const std::vector<std::string>& keys,                              \
      const std::vector<std::string>& op_keys) {                         \
    ASSERT_MSG(reps > 0, "reps shouldn't be 0");                         \
    turi::timer ti;                                                      \
    for (size_t ii = 0; ii < reps; ii++) {                               \
      _bench_test_aggreate_with_pool(nthreads, sf, (op), keys, op_keys); \
    }                                                                    \
    auto elapsed = ti.current_time_millis();                             \
    std::cout << "avg time to run w/ " << std::setw(2) << nthreads       \
              << " threads: " << elapsed / reps << " ms." << std::endl;  \
  }

// -------------------test suites using the framework ---------------------

/*
 * Testing count operation. This is a sample code.
 *
 * It's client's responsibility to make sure key and op_keys correct
 *
 * \param sf: input sframe
 * \param op: group-by aggregation operator
 * \param keys: compsite keys to group by
 * \param op_keys: columns to operate on
 * */
DEFINE_RUNNER(count, std::make_shared<groupby_operators::count>());

/*
 * control the output of a benchmark test case.
 *
 * the output is suggested to contain:
 * 1. nrows
 * 2. reps
 * 3. number of unique keys except count operator
 *
 * the generated sframe will be reused for different
 * sized thread pool settings.
 * */
void bench_test_aggreate_summary_count_bin(size_t nrows, size_t reps) {
  ASSERT_MSG(reps > 0, "reps shouldn't be 0");
  ASSERT_MSG(nrows > 0, "nrows shouldn't be 0");

  sframe sf = make_random_sframe(nrows, "b", false);

  std::cout << "=========== count on binary categorical data ============="
            << std::endl
            << std::endl;

  std::cout << "nrows: " << nrows << std::endl;
  std::cout << "reps: " << reps << std::endl;

  _bench_test_aggreate_runner(sf, reps, &bench_test_aggreate_fn_count,
                              sf.column_names(), {});

  std::cout << "========================== END ==========================="
            << std::endl
            << std::endl;
}

/*
 * Testing min operation. This is a sample code.
 *
 * It's client's responsibility to make sure key and op_keys correct
 *
 * \param sf: input sframe
 * \param op: group-by aggregation operator
 * \param keys: compsite keys to group by
 * \param op_keys: columns to operate on
 * */
DEFINE_RUNNER(min, std::make_shared<groupby_operators::min>());

/*
 * control the output of a benchmark test case.
 *
 * the output is suggested to contain:
 * 1. nrows
 * 2. reps
 * 3. number of unique keys except for count operator
 *
 * the generated sframe will be reused for different
 * sized thread pool settings.
 * */
void bench_test_aggreate_summary_min(size_t nrows, size_t reps, size_t nusers,
                                     int start, int end) {
  std::cout << "=========== bench_test_aggreate_min summary ============"
            << std::endl
            << std::endl;

  std::cout << "nrows: " << nrows << std::endl;
  std::cout << "reps: " << reps << std::endl;
  std::cout << "users: " << nusers << std::endl;

  auto sf_val = _generate_range_data<flex_int>(nrows, start, end, {"my_min"});

  {
    std::cout << "============= uniform distribution start ==============="
              << std::endl
              << std::endl;

    auto sf = _generate_range_data<flex_int>(nrows, 0, nusers, {"user_id"});
    sf = sf.add_column(sf_val.select_column(0), "my_min");

    _bench_test_aggreate_runner(sf, reps, &bench_test_aggreate_fn_min,
                                {"user_id"}, {"my_min"});

    std::cout << "=============== uniform distribution end ================"
              << std::endl
              << std::endl;
  }

  {
    std::cout << "============== skewed distribution start ================"
              << std::endl
              << std::endl;

    std::cout << "user_id '27' has " << std::setw(2) << 85
              << " percentage of appearance" << std::endl;
    std::cout << "user_id '35' has " << std::setw(2) << 7
              << " percentage of appearance" << std::endl;
    std::cout << "user_id '53' has " << std::setw(2) << 5
              << " percentage of appearance" << std::endl;
    std::cout << "user_id '08' has " << std::setw(2) << 3
              << " percentage of appearance" << std::endl
              << std::endl;

    auto sf = _generate_weighted_data<flex_int>(
        nrows, 0, nusers, {"user_id"}, {{85, 27}, {7, 35}, {5, 53}, {3, 8}});

    auto sf_val = _generate_range_data<flex_int>(nrows, start, end, {"my_min"});

    sf = sf.add_column(sf_val.select_column(0), "my_min");

    _bench_test_aggreate_runner(sf, reps, &bench_test_aggreate_fn_min,
                                {"user_id"}, {"my_min"});

    std::cout << "================ skewed distribution end ================="
              << std::endl
              << std::endl;
  }
}

/*
 * Testing average operation.
 *
 * It's client's responsibility to make sure key and op_keys correct
 *
 * \param sf: input sframe
 * \param nthreads: thread pool control
 * \param op: group-by aggregation operator
 * \param keys: compsite keys to group by
 * \param op_keys: columns to operate on
 * */
DEFINE_RUNNER(avg, std::make_shared<groupby_operators::average>());

void bench_test_aggreate_summary_avg(size_t nrows, size_t reps, size_t nusers,
                                     int start, int end) {
  ASSERT_MSG(reps > 0, "reps shouldn't be 0");

  std::cout << "=========== bench_test_aggreate_avg summary ============"
            << std::endl
            << std::endl;

  std::cout << "nrows: " << nrows << std::endl;
  std::cout << "reps: " << reps << std::endl;
  std::cout << "users: " << nusers << std::endl;

  auto sf = _generate_range_data<flex_int>(nrows, 0, nusers, {"user_id"});
  auto sf_val = _generate_range_data<flex_int>(nrows, start, end, {"my_avg"});

  sf = sf.add_column(sf_val.select_column(0), "my_avg");

  _bench_test_aggreate_runner(sf, reps, &bench_test_aggreate_fn_avg,
                              {"user_id"}, {"my_avg"});

  std::cout << "========================== END ==========================="
            << std::endl
            << std::endl;
}

};  // namespace
};  // namespace turi

int main(int argc, char** argv) {
  global_logger().set_log_level(LOG_PROGRESS);

  try {
    // 1 million rows
    size_t nrows = 100000;
    size_t reps = 3;
    size_t nusers = 1000;

    /* range */
    constexpr int start = -1000;
    constexpr int end = 1000;

    if (argc > 1) nrows = std::stoi(argv[1]);
    if (argc > 2) reps = std::stoi(argv[2]);
    if (argc > 3) nusers = std::stoi(argv[3]);
    if (argc > 4) turi::thread_size_beg = std::stoi(argv[4]);
    ASSERT_LE(turi::thread_size_beg, turi::thread::cpu_count());
    if (argc > 5)
      turi::sframe_debug_print = std::strlen(argv[5]) > 0 && argv[5][0] == 'T';

    /* use global var to control debug print a result sframe */

    /* actual benchmark runners */
    turi::bench_test_aggreate_summary_count_bin(nrows, reps);

    turi::bench_test_aggreate_summary_min(nrows, reps, nusers, start, end);

    turi::bench_test_aggreate_summary_avg(nrows, reps, nusers, start, end);

  } catch (...) {
    logstream(LOG_ERROR) << "bench_test_aggreate_count failed. pls check log"
                         << std::endl;
    return -1;
  }
  return 0;
}
