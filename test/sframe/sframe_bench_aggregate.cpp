/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#include <atomic>
#include <thread>
#include <core/storage/sframe_data/groupby_aggregate.hpp>
#include <core/storage/sframe_data/groupby_aggregate_operators.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/sframe_constants.hpp>
#include <iostream>

namespace turi {

using row_gen_func_t = std::function<std::vector<flexible_type>(size_t)>;

/* generate sframe */
/* I need to make this a fixture */
static sframe bench_test_sframe_generator(
    const std::vector<std::string>& column_names,
    const std::vector<flex_type_enum>& column_types, size_t nrows,
    row_gen_func_t next_row) {
  if (column_types.size() != column_names.size())
    log_and_throw("column_types size mismatches with column_names size");

  if (next_row(0).size() != column_types.size())
    log_and_throw(
        "row size mismatches with the output of row generate function");

  // calculate intervals
  size_t nthreads = thread::cpu_count();
  size_t interval = nrows / nthreads;
  std::vector<std::pair<size_t, size_t>> write_intervals;
  for (size_t ii = 0; ii < nthreads; ii++) {
    if (ii) {
      auto start = write_intervals.back().second;
      write_intervals.emplace_back(start, start + interval);
    } else {
      write_intervals.emplace_back(0, interval);
    }
  }
  write_intervals.back().second = nrows;

  // consturct sframe
  sframe out;
  out.open_for_write(column_names, column_types, "", nthreads);
  std::vector<turi::sframe_output_iterator> write_iters;
  write_iters.resize(nthreads);
  for (size_t ii = 0; ii < nthreads; ii++)
    write_iters[ii] = out.get_output_iterator(ii);

  parallel_for(0, write_iters.size(), [&](size_t ii) {
    size_t start = write_intervals[ii].first;
    size_t end = write_intervals[ii].second;
    auto out_iter = write_iters[ii];
    while (start < end) {
      *out_iter = next_row(start);
      start++;
    }
  });

  // finish writing
  out.close();

  return out;
}

static void bench_test_aggreate(const sframe& in_sf,
                                std::shared_ptr<group_aggregate_value> op,
                                size_t nthreads, bool debug_print = false) {
  if (nthreads == 0 || nthreads > thread::cpu_count())
    log_and_throw("invalid thread count");

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
  const std::string& key = in_sf.column_names().front();
  sframe out_sf =
      groupby_aggregate(in_sf, {key}, {"my_cnt"}, {{{}, op}});

  logstream(LOG_INFO) << "Bench test groupby with count finished in " << ti.current_time()
            << std::endl;

  // restore the default thread pool size
  if (need_to_set_thread_size) pool.resize(old_pl_sz);

  if (debug_print) out_sf.debug_print();
}

static void bench_test_aggreate_count(const sframe& sf, size_t nrows, size_t nthreads, size_t reps = 1) {
  if (reps == 0) {
    log_and_throw("reps shouldn't be 0");
  }

  std::cout << "=========== bench_test_aggreate_count summary ============"
            << std::endl;

  std::cout << "nrows: " << nrows << std::endl;
  std::cout << "nthreads: " << nthreads << std::endl;
  std::cout << "reps: " << reps << std::endl;

  turi::timer ti;
  for (size_t ii = 0; ii < reps; ii++)
    bench_test_aggreate(sf, std::make_shared<groupby_operators::count>(), nthreads);

  std::cout << "Elapsed time: " << ti.current_time() << " seconds" << std::endl;
  std::cout << "Average single pass: " << ti.current_time() / reps << " seconds" << std::endl;
  std::cout << std::endl;
  std::cout << "Elapsed time: " << ti.current_time_millis() << " ms" << std::endl;
  std::cout << "Average single pass: " << ti.current_time_millis() / reps << " ms" << std::endl;
  std::cout << "========================== END ==========================="
            << std::endl;

  return;
}

static void bench_test_aggreate_count_summary(size_t nrows, size_t reps) {
    auto binary_seq_gen = [=](size_t ii) -> std::vector<flexible_type> {
      return {flexible_type(static_cast<flex_int>(ii < nrows / 2))};
    };

    sframe sf = bench_test_sframe_generator(
        {"bin_val"}, {flex_type_enum::INTEGER}, nrows, binary_seq_gen);

    DASSERT_EQ(sf.num_rows(), nrows);
    // sf.debug_print();
    size_t max_hardware_mp = turi::thread::cpu_count();
    if (1 < max_hardware_mp) turi::bench_test_aggreate_count(sf, nrows, 1, reps);
    if (2 < max_hardware_mp) turi::bench_test_aggreate_count(sf, nrows, 2, reps);
    if (4 < max_hardware_mp) turi::bench_test_aggreate_count(sf, nrows, 4, reps);
    if (8 < max_hardware_mp) turi::bench_test_aggreate_count(sf, nrows, 8, reps);
    turi::bench_test_aggreate_count(sf, nrows, max_hardware_mp, reps);
}

};  // namespace turi

int main(int argc, char** argv) {
  try {
    // 1 million rows
    size_t nrows = 1000000;
    turi::bench_test_aggreate_count_summary(nrows, 100);
  } catch(...) {
    logstream(LOG_ERROR) << "bench_test_aggreate_count failed. pls check log" << std::endl;
    return -1;
  }
  return 0;
}
