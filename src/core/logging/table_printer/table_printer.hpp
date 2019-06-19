/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TABLE_PRINTER_H_
#define TURI_TABLE_PRINTER_H_

#include <core/logging/logger.hpp>
#include <core/logging/assertions.hpp>
#include <timer/timer.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/parallel/atomic.hpp>
#include <sstream>
#include <vector>

#include <core/util/code_optimization.hpp>
#include <core/logging/table_printer/table_element_printers.hpp>

namespace turi {

extern double MIN_SECONDS_BETWEEN_TICK_PRINTS;

/** A format specifying class telling the table printer to print the
 *  progress time.  See table_printer documentation for use.
 */
struct progress_time {
  explicit progress_time(const timer& tt) : progress_time(tt.current_time()) {}
  explicit progress_time(double seconds)  : elapsed_seconds(seconds) {}

  // For printing since the table start
  progress_time() : elapsed_seconds(-1) {}

  double elapsed_seconds;
};

/** A simple table printer for consistent information.
 *
 *  The constructor takes a list of (column name, width) strings.  If
 *  any column name is longer than the specified width, then the
 *  column is sized to just fit the header.
 *
 *  The header is printed with print_header().
 *
 *  Each row is shown with print_row(...), where ... contains the
 *  arguments of each row.  If this doesn't
 *
 *  numeric: double precision is printed so that it fits in the proper
 *    row width.  It is recommended that columns of floats be at least
 *    of width 8.  integers are printed without any decimal point.
 *
 *
 *  strings: strings are printed as is.  If they are longer than the
 *    specified line, they are truncated with a "..." printed after
 *    them.
 *
 *
 *  progress_time instance: This tells the printer to print the
 *    elapsed time.  If progress_time() is passed in without any
 *    arguments, then it prints the time elapsed since the class was
 *    constructed.  If progress_time() is constructed with a timer
 *    object or the elapsed number of seconds, then that is printed.
 *    See the example below for more information.
 *
 *  The footer is printed with print_footer().
 *
 * Example:
 *
 *
 *      progress_table_printer table( { {"Iteration", 0}, {"Time", 10}, {"RMSE", 8}, {"Top String", 16} } );
 *
 *      table.print_header();
 *
 *      table.print_row(0, progress_time(),           1e6,   "Alphabetical.");
 *      table.print_row(1, progress_time(),           10,    "Alphabet soup.");
 *      table.print_row(2, progress_time(0.1),        1,     "Mine!!!!");
 *      table.print_row(4, progress_time(100),        0.1,   "Now it's a really long string.");
 *      table.print_row(5, progress_time(1000),       0.01,  "Yours!!!!");
 *      table.print_row(6, progress_time(1000.0001),  0.001, "");
 *      table.print_row(7, progress_time(5e5),        1e-6,  "Turi");
 *
 *      table.print_row("FINAL", progress_time(5e6), 1e-6, "Turi");
 *
 *      table.print_footer();
 *
 * This prints out the table:
 *
 *      +-----------+------------+----------+------------------+
 *      | Iteration | Time       | RMSE     | Top String       |
 *      +-----------+------------+----------+------------------+
 *      | 0         | 15us       | 1000000  | Alphabetical.    |
 *      | 1         | 93us       | 10       | Alphabet soup.   |
 *      | 2         | 100ms      | 1        | Mine!!!!         |
 *      | 4         | 1m 40s     | 0.1      | Now it's a rea...|
 *      | 5         | 16m 40s    | 0.01     | Yours!!!!        |
 *      | 6         | 16m 40s    | 0.001    |                  |
 *      | 7         | 5d 18h 53m | 1e-06    | Turi             |
 *      | FINAL     | 57d 20h    | 1e-06    | Turi             |
 *      +-----------+------------+----------+------------------+
 *
 *
 * --------------------------------------------------------------------------------
 * TIMED PROGRESS PRINTING
 * --------------------------------------------------------------------------------
 *
 *
 * Using the print_progress_row(...) method instead of print_row(...),
 * you can control how often progress messages are printed.  It
 * automatically adjusts the printing interval so that the rows are
 * printed, on average, every 1-5 seconds.  The first argument to
 * print_progress_row is the tick variable, which must simply be a
 * monotonically increasing integer.
 *
 *
 * Example 1:
 *
 *   Code:
 *
 *        table_printer table( { {"Iteration", 0}, {"Elapsed Time", 10}, {"RMSE", 8} } );
 *
 *        table.print_header();
 *
 *        for(size_t i = 0; i < 2000; ++i) {
 *          table.print_progress_row(i, i, progress_time(), std::exp(-double(i) / 5000));
 *          usleep(8000); // Sleep for 8 milliseconds
 *        }
 *
 *        table.print_row("FINAL", progress_time(), 1e-6);
 *
 *        table.print_footer();
 *
 *   Output:
 *
 *       +-----------+--------------+----------+
 *       | Iteration | Elapsed Time | RMSE     |
 *       +-----------+--------------+----------+
 *       | 0         | 30us         | 1        |
 *       | 500       | 4.05s        | 0.904837 |
 *       | 1000      | 8.10s        | 0.818731 |
 *       | 1500      | 12.15s       | 0.740818 |
 *       | FINAL     | 16.21s       | 1e-06    |
 *       +-----------+--------------+----------+
 *
 * Example 2:
 *
 *    Code:
 *
 *        random::seed(0);
 *
 *        table_printer table( { {"samples_processed", 0}, {"Elapsed Time", 10}, {"A value", 8} } );
 *
 *        table.print_header();
 *
 *        size_t proc = 0;
 *
 *        for(size_t i = 0; i < 50000; ++i) {
 *          table.print_progress_row(proc, proc, progress_time(), i);
 *          proc += random::fast_uniform<size_t>(0, 100);
 *          usleep(100);  // sleep for 200 microseconds
 *        }
 *
 *        table.print_row("FINAL", progress_time(), 1e-6);
 *
 *        table.print_footer();
 *
 *    Output:
 *
 *        +-------------------+--------------+----------+
 *        | samples_processed | Elapsed Time | A value  |
 *        +-------------------+--------------+----------+
 *        | 0                 | 71us         | 0        |
 *        | 500081            | 1.61s        | 10009    |
 *        | 1000094           | 3.16s        | 19906    |
 *        | 1500049           | 4.78s        | 29980    |
 *        | 2000052           | 6.40s        | 39882    |
 *        | 2500054           | 8.03s        | 49971    |
 *        | FINAL             | 8.04s        | 1e-06    |
 *        +-------------------+--------------+----------+
 *
 *
 * --------------------------------------------------------------------------------
 * TRACKING
 * --------------------------------------------------------------------------------
 *
 * Optionally, the table printer can track the calls to the progress
 * row functions by storing each call as a row in an SFrame that can
 * be retrieved at the end.  This is useful for recording the training
 * statistics of an algorithm for reporting at the end with
 * get_tracked_table().
 *
 * Not every call to the progress functions are printed; the on-screen
 * printing is designed for a pleasing visual report.  Tracking,
 * however, is determined by an interval specified in the constructor
 * -- a row is recorded in the sframe every track_interval calls to
 * one of the progress printing calls.  It may be turned off by
 * setting track_interval to be 0.
 *
 */
class table_printer {

 public:
  /** Constructor.  Must be initialized elsewise using copy assignment ops. */
  table_printer(){}

  /** Constructor.  Sets up the columns.
   *
   * \param _format A vector of (column name, width) pairs.  If the
   * length of column name is larger than width, than width is set to
   * the column name.  See class header for examples.
   *
   * The track_interval determines how often a result is stored in the
   * SFrame tracking the row progress.  Every track_interval calls to
   * get_tracked_table, the row is written to the sframe.  If
   * track_interval is 0, then tracking is turned off.
   */
  table_printer(const std::vector<std::pair<std::string, size_t> >& _format,
                size_t track_interval = 1);


  /**  Need to clean up some things.
   */
  ~table_printer();

  /** Sets the output stream to something custom.  Only a reference to
   *  this stream is stored, so the stream must be in existance for as
   *  long as this class is.
   */
  void set_output_stream(std::ostream& out_stream) {
    alt_output_stream = &out_stream;
  }


  /** Prints the header.
   *
   *  Example output:
   *
   *      +-----------+------------+----------+------------------+
   *      | Iteration | Time       | RMSE     | Top String       |
   *      +-----------+------------+----------+------------------+
   */
  void print_header() const;

  /** Prints a line break.
   *
   *  Example output:
   *
   *      +-----------+------------+----------+------------------+
   */
  void print_line_break() const;

  /** Prints the footer.
   *
   *  Example output:
   *
   *      +-----------+------------+----------+------------------+
   */
  void print_footer() const;

  /** Print a row.
   *
   *  Example output:
   *
   *      table.print_row(5, progress_time(1000), 0.01,  "Yours!!!!");
   *
   *  Gives
   *
   *      | 5         | 16m 40s    | 0.01     | Yours!!!!        |
   */
  template <typename... Args>
  void print_row(const Args&... columns) const {
    ASSERT_EQ(size_t(sizeof...(Args)), format.size());

    std::ostringstream ss;
    size_t column_index = 0;

    ss << '|';

    _add_values_in_row(ss, column_index, columns...);

    _p(ss);
  }

  /** Same as print row but take a vector of one particular type.  May
   *  be flexible type. **/
  template <typename T>
  void print_row(const std::vector<T>& row_string) const {
    ASSERT_EQ(row_string.size(), format.size());

    std::ostringstream ss;

    ss << '|';

    for (size_t i = 0; i < row_string.size(); ++i) {
      os_log_value(i, row_string[i]);
      _get_table_printer(row_string[i]).print(ss, format[i].second);
    }

    _p(ss);
  }

  // If it's time to print the next row.  This can avoid doing
  // expensive things in an inner loop.
  GL_HOT_INLINE
  bool time_for_next_row() const {
    double time_ms = lowres_tt.ms();
    return (time_ms >= next_timed_print);
  }

  /** Print a row associated with the progress of an algorithm, but
   *  print at most once a second.
   */
  template <typename... Args>
  inline GL_HOT_INLINE void print_timed_progress_row(const Args&... columns) {

    double time_ms = lowres_tt.ms();

    if(time_ms >= next_timed_print) {
      std::lock_guard<mutex> pl_guard(print_lock);

      if(time_ms < next_timed_print)
        return;

      if(next_timed_print == -1) {
        next_timed_print = time_ms + 1000.0 * MIN_SECONDS_BETWEEN_TICK_PRINTS;
      } else {

        next_timed_print += 1000.0 * MIN_SECONDS_BETWEEN_TICK_PRINTS;

        // If that wasn't good enough
        if(next_timed_print < time_ms)
          next_timed_print = time_ms + 1000.0 * MIN_SECONDS_BETWEEN_TICK_PRINTS;
      }

      _print_progress_row(columns...);

      // Turn off the tracking if people don't want it, i.e. by
      // passing 0 as the track interval to the constructor.
      if(track_interval != 0)
        _track_progress(/* was_printed */ true, columns...);
    }
  }

  /** Print a row associated with the progress of an algorithm.
   *
   * The first argument is the tick, which can be something like
   * samples processed or iterations.  The time at which to update
   * this is automatically determined based on the first 2 (or more,
   * if calls are extremely frequent) calls.
   *
   *  Example output:
   *
   *      table.print_row(5,  5, progress_time(1000), 0.01,  "Yours!!!!");
   *
   *  Gives
   *
   *      | 5         | 16m 40s    | 0.01     | Yours!!!!        |
   */
  template <typename... Args>
  inline GL_HOT_INLINE void print_progress_row(size_t tick, const Args&... columns) {

    size_t ticks_so_far = ++num_ticks_so_far;
    bool was_printed = false;

    if(register_tick(tick, ticks_so_far)) {
      std::unique_lock<mutex> pl_guard(print_lock, std::defer_lock);
      if(pl_guard.try_lock()) {
        _print_progress_row(columns...);
        was_printed = true;
      }
    }

    if((track_interval != 0) && ((ticks_so_far - 1) % track_interval) == 0) {
      _track_progress(was_printed, columns...);
    }
  }

  /** Print a row associated with the progress of an algorithm.
   *
   * The first argument is the tick, which can be something like
   * samples processed or iterations.  The time at which to update
   * this is automatically determined based on the first 2 (or more,
   * if calls are extremely frequent) calls.
   *
   */
  inline GL_HOT_INLINE void print_progress_row_strs(size_t tick, const std::vector<std::string>& cols) {
    ASSERT_EQ(cols.size(), format.size());

    size_t ticks_so_far = ++num_ticks_so_far;
    bool was_printed = false;

    if(register_tick(tick, ticks_so_far)) {
      std::lock_guard<mutex> pl_guard(print_lock);
      print_row(cols);
      was_printed = true;
    }

    if (track_interval != 0 && ((ticks_so_far - 1) % track_interval) == 0) {
      std::lock_guard<mutex> guard(track_register_lock);

      if (!tracker_is_initialized) {
        track_row_values_.resize(cols.size());
        track_row_styles_.resize(cols.size());
      }

      for (size_t i = 0; i < cols.size(); ++i) {
        track_row_values_[i] = cols[i];  // Convert from string to flexible_type
        track_row_styles_[i] = style_type::kDefault;
      }

      track_progress_row(track_row_values_);
      track_row_was_printed_ = was_printed;
    }
  }



  /** Returns the elapsed time since class creation.  This is the
   *  value used if progress_time() is passed in to print_row.
   */
  double elapsed_time() const;


  /** Returns the current tracked table.  Any rows added after this is
   *  called will cause the table to be cleared and all rows to be
   *  added to another table.
   *
   */
  sframe get_tracked_table();

private:

  /**
   * Methods to log specific value types
   */
  static void os_log_value(size_t column_index, unsigned long long value);
  static void os_log_value(size_t column_index, unsigned long value);
  static void os_log_value(size_t column_index, unsigned int value);
  static void os_log_value(size_t column_index, long long value);
  static void os_log_value(size_t column_index, long value);
  static void os_log_value(size_t column_index, int value);
  static void os_log_value(size_t column_index, double value);
  static void os_log_value(size_t column_index, float value);
  void os_log_value(size_t column_index, const progress_time& value) const;
  static void os_log_value(size_t column_index, const char* value);
  static void os_log_value(size_t column_index, bool value);
  static void os_log_value(size_t column_index, const flexible_type& value);


  /** Returns the table_printer::style_type for a given value
   */
  template <typename T>
  table_internal::table_printer_element_base::style_type
  _get_table_printer_style(const T& v) {
    static_assert(table_internal::table_printer_element<T>::valid_type,
                  "Table printer not available for this type; please cast to approprate type.");

    return table_internal::table_printer_element<T>::style;
  }

  /** By value, disambiguate the elements
   */
  template <typename T>
  table_internal::table_printer_element<T> _get_table_printer(const T& v) const {
    static_assert(table_internal::table_printer_element<T>::valid_type,
                  "Table printer not available for this type; please cast to approprate type.");

    return table_internal::table_printer_element<T>(v);
  }

  /** Special case this one since it has to have access to the current
   *  local time, stored in this class.
   */
  table_internal::table_printer_element<progress_time> _get_table_printer(const progress_time& pt) const {

    double t = (pt.elapsed_seconds < 0) ? tt.current_time() : pt.elapsed_seconds;
    return table_internal::table_printer_element<progress_time>(t);
  }

  /** Recursively add values in a row.
   */
  template <typename T, typename... Args>
  GL_HOT_INLINE_FLATTEN void _add_values_in_row(std::ostringstream& ss, size_t column_index,
                                                const T& t, const Args&... columns) const {

    os_log_value(column_index, t);
    _get_table_printer(t).print(ss, format[column_index].second);
    _add_values_in_row(ss, column_index + 1, columns...);
  }

  /** Recursively add values in a row -- final termination state.
   */
  template <typename T, typename... Args>
  GL_HOT_INLINE_FLATTEN void _add_values_in_row(std::ostringstream& ss, size_t column_index, const T& t) const {
    os_log_value(column_index, t);
    _get_table_printer(t).print(ss, format[column_index].second);
  }

  ////////////////////////////////////////////////////////////////////////////////
  //

  template <typename... Args>
  GL_HOT_NOINLINE void _print_progress_row(const Args&... columns) {

    print_row(columns...);
  };

 private:

  /** Do the printing (in one place, since it's line origin is printed
   *  in debug mode.
   */
  void _p(std::ostringstream& ss) const {

    if(alt_output_stream == nullptr) {
      logprogress_stream << ss.str() << std::endl;
    } else {
      (*alt_output_stream) << ss.str() << std::endl;
    }
  }

 private:
  std::vector<std::pair<std::string, size_t> > format;

  timer tt;
  rdtsc_time lowres_tt;

  ////////////////////////////////////////
  // Controlling the output printing

  std::ostream* alt_output_stream = nullptr;

  //////////////////////////////////////////
  //  Controlling interval printing of things.

  atomic<double> time_of_first_tick;
  atomic<size_t> value_of_first_tick;

  atomic<size_t> num_ticks_so_far;
  atomic<size_t> next_tick_to_print;
  size_t tick_interval = 0;

  mutex print_lock;
  mutex tick_interval_lock;


  double next_timed_print = -1;


  /**  The ticks_so_far thing is stored in the
   *
   */
  inline bool register_tick(size_t tick, size_t ticks_so_far) {

    // RULES:
    //
    //   1. Always print the first 5 ticks seen.
    //
    //   2. On the fifth row printed, choose a schedule based on how
    //   long those took.  After that, some intervals will be always
    //   printed, but then the next_tick_to_print option will
    //   determine which ones to print

    if(ticks_so_far == 1) {
      value_of_first_tick = tick;
      time_of_first_tick = tt.current_time();

      return true;

    } else if(ticks_so_far < 5) {
      return true;

    } else if (ticks_so_far == 5) {

      // Make sure the ticks_so_far == 1 case has written this correctly.
      while(time_of_first_tick == -1.0);

      tick_interval = set_up_time_printing_interval(tick);

      // Set this to the next multiple of tick_interval after tick.
      size_t nttp = ( (tick + 1) + tick_interval);
      size_t rounded_nttp = nttp - nttp % tick_interval;
      if(rounded_nttp <= tick)
        rounded_nttp += tick_interval;

      // This unlocks any other threads hung in the while loop in the >5 case.
      next_tick_to_print = rounded_nttp;

      // Print this row.
      return true;

      // Now the typical case
    } else if(ticks_so_far > 5) {

      // Wait for the ticks_so_far == 5 thread to finish setting everything.
      if(UNLIKELY(next_tick_to_print == 0)) {
        while(next_tick_to_print == 0);
      }

      size_t next_tick = next_tick_to_print;

      if(tick < next_tick) {
        return always_print(tick);

      } else {

        DASSERT_GT(tick_interval, 0);

        std::unique_lock<mutex> til_guard(tick_interval_lock, std::defer_lock);

        if (til_guard.try_lock()) {
        if(tick < next_tick_to_print) {
            return always_print(tick);
        } else {

          while(next_tick_to_print <= tick)
            next_tick_to_print += tick_interval;

          return true;
        }
        } else {
          return false;
        }
      }

    }

    return true;
  }

  /**  Sets up the time interval at which things are printed.
   */
  size_t set_up_time_printing_interval(size_t tick);


  /** Returns true if the given tick should always be printed.  This
   *  prevents the common case of models going too quickly to actually print results
   *
   */
  inline GL_HOT_INLINE_FLATTEN bool always_print(size_t tick_index) {

    // Always print 1,2,3,4,5
    if(tick_index <= 5)
      return true;

    // Always print 10, 50, 100, 500, 1000, 5000, etc.

    while( (tick_index % 10) == 0)
      tick_index /= 10;

    return (tick_index == 1 || tick_index == 5);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Stuff for registering the values

  using style_type = table_internal::table_printer_element_base::style_type;

  mutable mutex track_register_lock;
  sframe track_sframe;
  bool tracker_is_initialized = false;
  bool track_row_was_printed_ = false;
  sframe::iterator tracking_out_iter;
  std::vector<flexible_type> track_row_values_;
  std::vector<style_type> track_row_styles_;
  size_t track_interval = 1;


  /**  Record a row in the tracking SFrame.
   */
  template <typename... Args>
  inline GL_HOT_NOINLINE void _track_progress(
      bool was_printed, const Args&... columns) {

    std::lock_guard<decltype(track_register_lock)> register_lock_gourd(track_register_lock);

    const size_t n = sizeof...(columns);

    DASSERT_EQ(n, format.size());

    if(!tracker_is_initialized) {
      track_row_values_.resize(n);
      track_row_styles_.resize(n);
    }

    _register_values_in_row(0, columns...);
    track_progress_row(track_row_values_);
    track_row_was_printed_ = was_printed;
  }

  void print_track_row_if_necessary() const;

  inline GL_HOT_NOINLINE void
  track_progress_row(const std::vector<flexible_type>& track_row_buffer) {

    size_t n = track_row_buffer.size();
    if(!tracker_is_initialized) {
      track_sframe = sframe();

      // Get the names
      std::vector<std::string> column_names(n);
      std::vector<flex_type_enum> column_types(n);

      for(size_t i = 0; i < n; ++i) {
        column_names[i] = format[i].first;
        column_types[i] = track_row_buffer[i].get_type();
      }

      track_sframe.open_for_write(column_names, column_types, "", 1);
      tracking_out_iter = track_sframe.get_output_iterator(0);
      tracker_is_initialized = true;
    }

    *tracking_out_iter = track_row_buffer;
    ++tracking_out_iter;
  }

  /** Recursively add values in a row.
   */
  template <typename T, typename... Args>
  GL_HOT_INLINE_FLATTEN void _register_values_in_row(
      size_t column_index, const T& t, const Args&... columns) {
    DASSERT_LT(column_index, track_row_values_.size());
    DASSERT_LT(column_index, track_row_styles_.size());

    track_row_values_[column_index] = _get_table_printer(t).get_value();
    track_row_styles_[column_index] = _get_table_printer_style(t);
    _register_values_in_row(column_index + 1, columns...);
  }

  /** Recursively add values in a row -- final termination state.
   */
  template <typename T, typename... Args>
  GL_HOT_INLINE_FLATTEN void _register_values_in_row(
      size_t column_index, const T& t) {
    DASSERT_LT(column_index, track_row_values_.size());
    DASSERT_LT(column_index, track_row_styles_.size());

    track_row_values_[column_index] = _get_table_printer(t).get_value();
    track_row_styles_[column_index] = _get_table_printer_style(t);
  }

};

}

#endif /* TURI_TABLE_PRINTER_H_ */
