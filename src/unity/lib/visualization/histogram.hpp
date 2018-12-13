/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef __TC_HISTOGRAM
#define __TC_HISTOGRAM

#include <sframe/groupby_aggregate_operators.hpp>
#include <unity/lib/visualization/histogram.hpp>
#include <unity/lib/visualization/plot.hpp>
#include <unity/lib/visualization/vega_spec.hpp>
#include <unity/lib/gl_sarray.hpp>

#include "transformation.hpp"

namespace turi {
namespace visualization {

/*
 * histogram_bins()
 * Represents bin values (typically rescaled from original bin contents)
 * along with an effective range (min of first bin, last of max)
 */
struct histogram_bins {
  flex_list bins;
  double max;
  double min;
};

/*
 * histogram_result()
 *
 * Stores the intermediate or complete result of a histogram stream.
 *
 * Attributes
 * ----------
 * bins : flex_list<flex_int>
 *     The counts in each bin, in the range scale_min to scale_max.
 * min : flexible_type
 *     The lowest value encountered in the source SArray.
 * max : flexible_type
 *     The highest value encountered in the source SArray.
 * scale_min : double
 *     The low end of the range represented by the bins.
 * scale_max : double
 *     The high end of the range represented by the bins.
 *
 * Methods
 * -------
 * init(flexible_type value1, flexible_type value2)
 *     Initialize the result with the range represented by two values.
 * init(flexible_type value1, flexible_type value2, double scale1, double scale2)
 *     Initialize the result with the range represented by two values and
 *     the scale range (should be >= value range) represented by two scale
 *     values.
 * rescale(double new_min, double new_max)
 *     Rescale the result to the range represented by the provided values.
 */
struct histogram_result : public sframe_transformation_output {
  public:
    histogram_result();
    flex_type_enum m_type;
    constexpr static size_t MAX_BINS = 1000;
    std::array<flex_int, MAX_BINS> bins;
    flexible_type min;
    flexible_type max;
    double scale_min;
    double scale_max;
    void rescale(double new_min, double new_max);
    void init(flex_type_enum dtype, flexible_type value1, flexible_type value2);
    void init(flex_type_enum dtype, flexible_type value1, flexible_type value2, double scale1, double scale2);
    histogram_bins get_bins(flex_int num_bins) const;
    flexible_type get_min_value() const;
    flexible_type get_max_value() const;
    void add_element_simple(const flexible_type& value); // updates the result w/ value
    virtual std::string vega_column_data(bool) const override;
    virtual std::string vega_summary_data() const override;

    // also store and compute basic summary stats
    groupby_operators::count m_count; // num rows
    groupby_operators::count_distinct m_count_distinct; // num unique
    groupby_operators::non_null_count m_non_null_count; // (inverse) num missing
    groupby_operators::average m_average; // mean
    groupby_operators::min m_min; // min
    groupby_operators::max m_max; // max
    groupby_operators::quantile m_median; // median (quantile at 0.5)
    groupby_operators::stdv m_stdv; // stdev
};

/*
 * histogram()
 *
 * Implements Optimal Streaming Histogram (sort-of) as described in
 * http://blog.amplitude.com/2014/08/06/optimal-streaming-histograms/.
 * dtype of sarray can be flex_int or flex_float
 * histogram always gives bins as flex_ints (bin counts are positive integers).
 *
 * Attributes
 * ----------
 *
 * Methods
 * ----------
 * init(const gl_sarray& source)
 *     Initialize the histogram with an SArray as input.
 * eof()
 *     Returns true if the streaming histogram has covered all input, false
 *     otherwise.
 * get()
 *     Bins (up to) the next BATCH_SIZE values from the input, and returns
 *     the histogram (result type, as shown below) representing the current
 *     distribution of values seen so far.
 */
typedef transformation<gl_sarray, histogram_result> histogram_parent;
class histogram : public histogram_parent {
  public:
    virtual std::vector<histogram_result> split_input(size_t num_threads) override;
    virtual void merge_results(std::vector<histogram_result>& transformers) override;
    virtual void init(const gl_sarray& source, size_t batch_size) override;
};

std::shared_ptr<Plot> plot_histogram(
  gl_sarray& sa, std::string xlabel, std::string ylabel, 
  std::string title);

}}

#endif // __TC_HISTOGRAM
