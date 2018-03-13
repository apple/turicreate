/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <logger/logger.hpp>
#include <logger/assertions.hpp>
#include <timer/timer.hpp>
#include <sstream>
#include <util/try_finally.hpp>
#include <table_printer/table_printer.hpp>

namespace turi {

/** Constructor.  Sets up the columns.
 *
 * \param _format A vector of (column name, width) pairs.  If the
 * length of column name is larger than width, than width is set to
 * the column name.  See class header for examples.
 */
table_printer::table_printer(const std::vector<std::pair<std::string, size_t> >& _format,
                             size_t _track_interval)
    : format(_format)
    , time_of_first_tick(-1.0)
    , value_of_first_tick(0)
    , num_ticks_so_far(0)
    , next_tick_to_print(0)
    , track_interval(_track_interval)
{
  DASSERT_FALSE(format.empty());

  size_t total_size = 0;
  
  // The column start.
  total_size += 2;

  for(size_t i = 0; i < format.size(); ++i) {

    if(format[i].first.size() > format[i].second)
      format[i].second = format[i].first.size();

    total_size += format[i].second;
    total_size += 3; // Width into next column.
  }

  lowres_tt.ms();
  tt.start();
}

/**  Need to clean up some things.
 */
table_printer::~table_printer() {
  if(track_sframe.is_opened_for_write()) {
    track_sframe.close();
  }
}

/** Prints the header.
 *
 *  Example output:
 *
 *      +-----------+------------+----------+------------------+
 *      | Iteration | Time       | RMSE     | Top String       |
 *      +-----------+------------+----------+------------------+
 */
void table_printer::print_header() const {
  print_line_break();

  std::ostringstream ss;

  ss << '|';

  for(const auto& p : format) {

    ss << ' ';

    ss << p.first;

    for(size_t i = p.first.size(); i < p.second; ++i)
      ss << ' ';

    ss << ' ' << '|';
  }

  _p(ss);

  print_line_break();
}


/** Prints a line break.
 *
 *  Example output:
 *
 *      +-----------+------------+----------+------------------+
 */
void table_printer::print_line_break() const {

  std::ostringstream ss;

  ss << '+';

  for(const auto& p : format) {
    for(size_t i = 0; i < p.second + 2; ++i)
      ss << '-';

    ss << '+';
  }

  _p(ss);
}

/** Prints the footer.
 *
 *  Example output:
 *
 *      +-----------+------------+----------+------------------+
 */
void table_printer::print_footer() const {
  print_line_break();
}

/** Returns the elapsed time since class creation.  This is the
 * value used if progress_time() is passed in to print_row.
 */
double table_printer::elapsed_time() const {
  return tt.current_time();
}

/** Returns the current tracked table.  Any rows added after this is
 *  called will cause the table to be cleared and all rows to be
 *  added to another table.
 */
sframe table_printer::get_tracked_table() {

  std::lock_guard<decltype(track_register_lock)> register_lock_guard(track_register_lock);

  if(!tracker_is_initialized) {

    // Initialize an empty table
    track_sframe = sframe();

    size_t n = format.size();

    // Get the names
    std::vector<std::string> column_names(n);
    std::vector<flex_type_enum> column_types(n);

    for(size_t i = 0; i < n; ++i) {
      column_names[i] = format[i].first;
      column_types[i] = flex_type_enum::STRING;
    }

    track_sframe.open_for_write(column_names, column_types, "", 1);
    tracking_out_iter = track_sframe.get_output_iterator(0);
    tracker_is_initialized = true;
  }
  
  if(track_sframe.is_opened_for_write()) {
    track_sframe.close();
  }

  tracker_is_initialized = false;
  
  return track_sframe;
}

/**  Sets up the time interval at which things are printed.
 */
size_t table_printer::set_up_time_printing_interval(size_t tick) {
    
  DASSERT_EQ(size_t(next_tick_to_print), 0); 

  double time_since_first_tick_registration
      = (tt.current_time() - time_of_first_tick);

  double time_estimate_between_ticks
      = time_since_first_tick_registration / (tick - value_of_first_tick);

  // Now, time to set the tick interval
  size_t _tick_interval = 1000000000;

  size_t tick_candidates[] = {1, 5, 10, 25};
  bool tick_interval_set = false; 
        
  for(size_t magnitude = 0; !tick_interval_set && magnitude < 10; ++magnitude) {

    for(size_t itv_itvl : tick_candidates) {
      if(itv_itvl * time_estimate_between_ticks >= MIN_SECONDS_BETWEEN_TICK_PRINTS) {
        _tick_interval = itv_itvl;
        tick_interval_set = true;
        break;
      }
    }

    // Multiply by 10 and try again, so we're testing 1, 5, 10, 25,
    // 50 100 250 500, etc.  The min is what matters.
    for(size_t& itv_itvl : tick_candidates)
      itv_itvl *= 10;
  }

  return _tick_interval; 
}

}
