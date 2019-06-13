/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/logging/logger.hpp>
#include <core/logging/assertions.hpp>
#include <timer/timer.hpp>
#include <sstream>
#include <core/util/try_finally.hpp>
#include <core/globals/globals.hpp>
#include <core/logging/table_printer/table_printer.hpp>

#ifdef __APPLE__
#include <os/log.h>
#undef MIN
#undef MAX

static os_log_t& os_log_object() {
  static os_log_t log_object = os_log_create("com.apple.turi", "table_printer");
  return log_object;
}

#define _os_log_event_impl(event)                                         \
  os_log_info(                                                            \
    os_log_object(),                                                      \
    "event: %lu",                                                         \
    event)

#define _os_log_event_with_value_impl(format, event, column_index, value) \
  os_log_info(                                                            \
    os_log_object(),                                                      \
    format,                                                               \
    event,                                                                \
    column_index,                                                         \
    value)

#define _os_log_header_impl(format, column_index, value)                  \
  _os_log_event_with_value_impl(format, 1ul, column_index, value)

#define _os_log_value_impl(format, column_index, value)                   \
  _os_log_event_with_value_impl(format, 2ul, column_index, value)

#else
#define _os_log_event_impl(...)
#define _os_log_header_impl(...)
#define _os_log_value_impl(...)
#endif


namespace turi {

  EXPORT double MIN_SECONDS_BETWEEN_TICK_PRINTS = 3.0;

  REGISTER_GLOBAL(double, MIN_SECONDS_BETWEEN_TICK_PRINTS, true);

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
  _os_log_event_impl(0ul /* event: started */);

  print_line_break();
  std::ostringstream ss;
  ss << '|';

  size_t i = 0;
  for(const auto& p : format) {

    ss << ' ';

    ss << p.first;

    for(size_t i = p.first.size(); i < p.second; ++i)
      ss << ' ';

    ss << ' ' << '|';

    _os_log_header_impl(
      "event: %lu, column: %lu, value: %{public}s",
      i, /* column_index */
      p.first.c_str());

    i++;
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
  // If tracking is enabled, print the last tracked row if it wasn't already
  // printed. Ensures the final row is printed if the tracking interval is 1.
  print_track_row_if_necessary();

  print_line_break();

  _os_log_event_impl(3ul /* event: ended */);
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

void table_printer::print_track_row_if_necessary() const {
  std::lock_guard<mutex> guard(track_register_lock);
  if (track_row_was_printed_) return;
  if (track_row_values_.empty()) return;

  ASSERT_EQ(track_row_values_.size(), format.size());
  ASSERT_EQ(track_row_styles_.size(), format.size());

  std::ostringstream ss;

  ss << '|';
  for (size_t i = 0; i < track_row_values_.size(); ++i) {
    const flexible_type& value = track_row_values_[i];
    os_log_value(i, value);
    const size_t width = format[i].second;

    // Use track_row_styles_ to recover any type information lost when stuffing
    // the value into flexible_type.
    switch (track_row_styles_[i]) {
    case style_type::kDefault:
      _get_table_printer(value).print(ss, width);
      break;
    case style_type::kBool:
      _get_table_printer(value.to<bool>()).print(ss, width);
      break;
    case style_type::kProgressTime:
      _get_table_printer(progress_time(value.to<double>())).print(ss, width);
      break;
    }
  }

  _p(ss);
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

void table_printer::os_log_value(size_t column_index, unsigned long long value) {
  _os_log_value_impl("event: %lu, column: %lu, value: %llu", column_index, value);
}

void table_printer::os_log_value(size_t column_index, unsigned long value) {
  _os_log_value_impl("event: %lu, column: %lu, value: %lu", column_index, value);
}

void table_printer::os_log_value(size_t column_index, unsigned int value) {
  _os_log_value_impl("event: %lu, column: %lu, value: %u", column_index, value);
}

void table_printer::os_log_value(size_t column_index, long long value) {
  _os_log_value_impl("event: %lu, column: %lu, value: %lld", column_index, value);
}

void table_printer::os_log_value(size_t column_index, long value) {
  _os_log_value_impl("event: %lu, column: %lu, value: %ld", column_index, value);
}

void table_printer::os_log_value(size_t column_index, int value) {
  _os_log_value_impl("event: %lu, column: %lu, value: %d", column_index, value);
}

void table_printer::os_log_value(size_t column_index, double value) {
  _os_log_value_impl("event: %lu, column: %lu, value: %f", column_index, value);
}

void table_printer::os_log_value(size_t column_index, float value) {
  _os_log_value_impl("event: %lu, column: %lu, value: %f", column_index, value);
}

void table_printer::os_log_value(size_t column_index, const progress_time& value) const {
  _os_log_value_impl("event: %lu, column: %lu, value: %f seconds",
    column_index,
    (value.elapsed_seconds < 0) ? tt.current_time() : value.elapsed_seconds);
}

void table_printer::os_log_value(size_t column_index, const char* value) {
  _os_log_value_impl("event: %lu, column: %lu, value: %{public}s", column_index, value);
}

void table_printer::os_log_value(size_t column_index, bool value) {
  _os_log_value_impl("event: %lu, column: %lu, value: %hhu", column_index, static_cast<unsigned char>(value));
}

void table_printer::os_log_value(size_t column_index, const flexible_type& value) {
  switch (value.get_type()) {
    case flex_type_enum::INTEGER:
      _os_log_value_impl("event: %lu, column: %lu, value: %lld", column_index, value.get<flex_int>());
      break;
    case flex_type_enum::DATETIME:
      _os_log_value_impl("event: %lu, column: %lu, value: %{time_t}lld", column_index, value.get<flex_date_time>().posix_timestamp());
      break;
    case flex_type_enum::FLOAT:
      _os_log_value_impl("event: %lu, column: %lu, value: %f", column_index, value.get<flex_float>());
      break;
    case flex_type_enum::STRING:
      _os_log_value_impl("event: %lu, column: %lu, value: %{public}s", column_index, value.get<flex_string>().c_str());
      break;
    case flex_type_enum::VECTOR:
    case flex_type_enum::ND_VECTOR:
    case flex_type_enum::LIST:
    case flex_type_enum::DICT:
    case flex_type_enum::IMAGE:
    default:
      _os_log_value_impl("event: %lu, column: %lu, value: instance of complex type %{public}s", column_index, flex_type_enum_to_name(value.get_type()));
      break;
  }
}

}
