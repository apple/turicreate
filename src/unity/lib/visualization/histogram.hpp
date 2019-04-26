/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef __TC_HISTOGRAM
#define __TC_HISTOGRAM

#include <sframe/groupby_aggregate_operators.hpp>
#include <unity/lib/visualization/batch_size.hpp>
#include <unity/lib/visualization/escape.hpp>
#include <unity/lib/visualization/histogram.hpp>
#include <unity/lib/visualization/plot.hpp>
#include <unity/lib/visualization/vega_spec.hpp>
#include <unity/lib/gl_sarray.hpp>

#include "transformation.hpp"

#include <sstream>

namespace turi {
namespace visualization {

/*
 * histogram_bins()
 * Represents bin values (typically rescaled from original bin contents)
 * along with an effective range (min of first bin, last of max)
 */
template<typename T>
struct histogram_bins {
  flex_list bins;
  T max;
  T min;
};

/*
 * bin_specification_object
 * Represents bin widths (typically rescaled from original bin contents) for
 * compatibility with https://vega.github.io/vega/docs/scales/#bins
 */
template<typename T>
struct bin_specification_object {
  T start, stop, step;
  bin_specification_object(T start, T stop, T step)
  : start(start), stop(stop), step(step) {}
  void serialize(std::stringstream& ss) {
    ss << "{\"start\":" << start;
    ss << ", \"stop\":" << stop;
    ss << ", \"step\":" << step << "}";
  }
};

template<typename T>
static T get_value_at_bin(
    T bin_idx,
    T scale_min,
    T scale_max,
    T num_bins
    ) {
  double ret = (
    ((double)bin_idx / (double)num_bins) *
    (double)(scale_max - scale_min)
  ) + scale_min;
  if (std::is_same<T, flex_int>::value) {
    DASSERT_EQ(ret, std::floor(ret));
  }
  return ret;
}

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
 * scale_min : T
 *     The low end of the range represented by the bins.
 * scale_max : T
 *     The high end of the range represented by the bins.
 *
 * Methods
 * -------
 * init(flexible_type value1, flexible_type value2)
 *     Initialize the result with the range represented by two values.
 * init(flexible_type value1, flexible_type value2, T scale1, T scale2)
 *     Initialize the result with the range represented by two values and
 *     the scale range (should be >= value range) represented by two scale
 *     values.
 * rescale(T new_min, T new_max)
 *     Rescale the result to the range represented by the provided values.
 */
template<typename T>
struct histogram_result : public sframe_transformation_output {
  public:
    // also store and compute basic summary stats
    groupby_operators::count m_count; // num rows
    groupby_operators::count_distinct m_count_distinct; // num unique
    groupby_operators::non_null_count m_non_null_count; // (inverse) num missing
    groupby_operators::average m_average; // mean
    groupby_operators::min m_min; // min
    groupby_operators::max m_max; // max
    groupby_operators::quantile m_median; // median (quantile at 0.5)
    groupby_operators::stdv m_stdv; // stdev

    histogram_result() {
      // initialize quantile (so we can use it as median)
      m_median.init(std::vector<double>({0.5}));
    }
    flex_type_enum m_type;
    const static size_t VISIBLE_BINS = 20;
    const static size_t REAL_BINS = 1000;
    std::array<flex_int, REAL_BINS> bins;
    T min;
    T max;
    T scale_min;
    T scale_max;
    static size_t get_bin_idx(
        T value,
        T scale_min,
        T scale_max
      ) {
      T range = scale_max - scale_min;
      size_t bin = std::floor(
        (((double)value - (double)scale_min) / (double)range) *
        (double)histogram_result<T>::REAL_BINS
      );
      if (bin == histogram_result<T>::REAL_BINS) {
        bin -= 1;
      }
      DASSERT_LT(bin, histogram_result<T>::REAL_BINS);
      return bin;
    }
    void rescale(T new_min, T new_max) {
      if (std::is_same<T, flex_int>::value) {
        static_assert(REAL_BINS % 2 == 0, "Streaming int histogram expects REAL_BINS cleanly divisible by 2");
      }
      // collapse bins towards the center to expand range by 2x
      while (new_min < scale_min || new_max > scale_max) {
        // first, combine bins next to each other (every other bin)
        for (ssize_t i=(REAL_BINS / 2) - 1; i>0; i-=2) {
          bins[i] += bins[i-1];
        }
        for (size_t i=(REAL_BINS / 2); i<REAL_BINS; i+=2) {
          bins[i] += bins[i+1];
        }
        // then, collapse them inward towards the center
        for (size_t i=0; i<(REAL_BINS/4); i++) {
          bins[(REAL_BINS/2) + i] = bins[(REAL_BINS/2) + (2 * i)];
          bins[(REAL_BINS/2) - (i + 1)] = bins[(REAL_BINS/2) - ((2 * i) + 1)];
        }
        // finally, zero out the newly-unused bins
        if (std::is_same<T, flex_int>::value) {
          static_assert((REAL_BINS * 3) % 4 == 0, "Streaming int histogram expects (REAL_BINS*3) cleanly divisible by 4");
        }
        for (size_t i=((REAL_BINS * 3) / 4); i<REAL_BINS; i++) {
          bins[i] = 0;
        }
        if (std::is_same<T, flex_int>::value) {
          static_assert(REAL_BINS % 4 == 0, "Streaming int histogram expects REAL_BINS cleanly divisible by 4");
        }
        for (size_t i=0; i<(REAL_BINS/4); i++) {
          bins[i] = 0;
        }

        // bump up scale by 2x
        T range = scale_max - scale_min;
        if (std::is_same<T, flex_int>::value) {
          DASSERT_EQ((flex_int)range % 2, 0);
        }
        scale_max += (range / 2);
        scale_min -= (range / 2);
      }
    }
    void init(flex_type_enum dtype, flexible_type value1, flexible_type value2) {
      // use 2 initial values for both min/max and bin scale value1/value2
      this->init(dtype, value1, value2, value1, value2);
    }
    void init(
      flex_type_enum dtype, T value1, T value2, T scale1, T scale2
    ) {
      // initialize min/max to use dtype (for some reason it defaults to int, then crashes on float)
      this->m_type = dtype;
      this->m_min.set_input_type(dtype);
      this->m_max.set_input_type(dtype);

      // initialize bins to 0
      for (size_t i=0; i<this->bins.size(); i++) {
        this->bins[i] = 0;
      }

      T epsilon;
      if (std::is_same<T, flex_int>::value) {
        epsilon = 1;
      } else {
        epsilon = 1e-2;
      }

      this->min = std::min(value1, value2);
      this->max = std::max(value1, value2);
      this->scale_min = std::min(scale1, scale2);
      this->scale_max = std::max(scale1, scale2);
      if (this->scale_max == this->scale_min) {
        // make sure they are not the same value
        if(this->scale_max>0){
          this->scale_max *= (1.0+epsilon);
        }else if(this->scale_max<0){
          this->scale_max *= (1.0-epsilon);
        }else{ // scale_max == 0
          this->scale_max += epsilon;
        }
      }
      if (std::is_same<T, flex_int>::value) {
        // For int histogram, make sure our scale range is evenly divisible by 2 to start with
        if ((flex_int)(this->scale_max - this->scale_min) % 2 != 0) {
          this->scale_max += 1;
          DASSERT_EQ((flex_int)(this->scale_max - this->scale_min) % 2, 0);
        }

        // We also need to make sure we can evenly divide the scale range into REAL_BINS,
        // so that we don't end up with non-integer bin boundaries
        flex_int pad = (flex_int)(this->scale_max - this->scale_min) % REAL_BINS;
        if (pad > 0) {
          pad = REAL_BINS - pad;
          flex_int pad_left = pad / 2;
          flex_int pad_right = (pad / 2) + (pad % 2);
          this->scale_min -= pad_left;
          this->scale_max += pad_right;
        }
        DASSERT_EQ((flex_int)(this->scale_max - this->scale_min) % REAL_BINS, 0);
      }
    }
    histogram_bins<T> get_bins(flex_int num_bins) const {
      if (num_bins < 1) {
        log_and_throw("num_bins must be positive.");
      }
      histogram_bins<T> ret;

      // determine the bin range that covers min to max
      size_t first_bin = get_bin_idx(min, scale_min, scale_max);
      size_t last_bin = get_bin_idx(max, scale_min, scale_max);
      size_t effective_bins = (last_bin - first_bin) + 1;

      // Might end up with fewer effective bins due to very small
      // number of unique values. For now, comment out this assert.
      // TODO -- what should we assert here instead, to make sure we have enough
      // effective range for the desired number of bins? Or should we force
      // discrete histogram for very low cardinality? (At which point, we keep
      // this assertion).
      //DASSERT_GE(effective_bins, (REAL_BINS/4));

      if (static_cast<size_t>(num_bins) > (REAL_BINS/4)) {
        log_and_throw("num_bins must be less than or equal to the effective number of bins available.");
      }

      // rescale to desired bins, taking more than the effective range if
      // necessary in order to get to num_bins total without resampling
      size_t bins_per_bin = effective_bins / num_bins;
      size_t overflow = effective_bins % num_bins;
      size_t before = 0;
      size_t after = 0;
      if (overflow) {
        overflow = num_bins - overflow;
        bins_per_bin = (effective_bins + overflow) / num_bins;
        before = overflow / 2;
        after = (overflow / 2) + (overflow % 2);
      }

      ret.bins = flex_list(num_bins, 0); // initialize empty
      ret.min = get_value_at_bin<T>(std::max<ssize_t>(0, first_bin - before), scale_min, scale_max, REAL_BINS);
      ret.max = get_value_at_bin<T>(std::min<ssize_t>(last_bin + after + 1, REAL_BINS), scale_min, scale_max, REAL_BINS);
      for (size_t i=0; i<static_cast<size_t>(num_bins); i++) {
        for (size_t j=0; j<bins_per_bin; j++) {
          ssize_t idx = (i * bins_per_bin) + j + (first_bin - before);
          if (idx < 0 || static_cast<size_t>(idx) >= REAL_BINS) {
            // don't try to get values below 0, or past REAL_BINS, that would be silly
            continue;
          }
          ret.bins[i] += this->bins[idx];
        }
      }
      return ret;
    }
    flexible_type get_min_value() {
      return this->min;
    }
    flexible_type get_max_value() const {
      return this->max;
    }
    void add_element_simple(const flexible_type& value) {
      /*
      * add element to summary stats
      */
      m_count.add_element_simple(value);
      m_count_distinct.add_element_simple(value);
      m_non_null_count.add_element_simple(value);
      m_average.add_element_simple(value);
      m_min.add_element_simple(value);
      m_max.add_element_simple(value);
      m_median.add_element_simple(value);
      m_stdv.add_element_simple(value);

      /*
      * add element to histogram
      */


      if (value.get_type() == flex_type_enum::UNDEFINED) {
        return;
      }

      // ignore nan values
      // ignore inf values
      if (value.get_type() == flex_type_enum::FLOAT &&
          !std::isfinite(value.get<flex_float>())) {
        return;
      }

      // assign min/max to return value
      if (value < this->min) { this->min = value; }
      if (value > this->max) { this->max = value; }

      // resize bins if needed
      this->rescale(this->min, this->max);

      // update count in bin
      size_t bin = get_bin_idx(value, this->scale_min, this->scale_max);
      this->bins[bin] += 1;
    }
    virtual std::string vega_column_data(bool) const override {
      auto bins = get_bins(VISIBLE_BINS);
      T binWidth = (bins.max - bins.min)/VISIBLE_BINS;
      bin_specification_object<T> binSpec(bins.min, bins.max, binWidth);

      std::stringstream ss;

      for (size_t i=0; i<bins.bins.size(); i++) {
        if (i != 0) {
          ss << ",";
        }
        const auto& value = bins.bins[i];
        ss << "{\"left\": ";
        ss << static_cast<T>(bins.min + (i * binWidth));
        ss << ",\"right\": ";
        ss << static_cast<T>(bins.min + ((i+1) * binWidth));
        ss << ", \"count\": ";
        ss << value;
        ss << "}";
      }

      // if there are null values, include them separately
      size_t null_count = m_count.emit() - m_non_null_count.emit();
      if (null_count > 0) {
        ss << ",";
        ss << "{\"missing\": true";
        ss << ", \"count\": ";
        ss << null_count;
        ss << "}";
      }

      // include metadata about bin ranges
      ss << ",";
      binSpec.serialize(ss);

      return ss.str();
    }
    virtual std::string vega_summary_data() const override {
      std::stringstream ss;

      flex_vec median_vec = m_median.emit().template get<flex_vec>();
      flex_float median = median_vec[0];
      flex_int num_missing = m_count.emit() - m_non_null_count.emit();
      std::string data = vega_column_data(true);
      std::string typeName = flex_type_enum_to_name(m_type);

      ss << "\"type\": \"" << typeName << "\",";
      ss << "\"num_unique\": " << m_count_distinct.emit() << ",";
      ss << "\"num_missing\": " << num_missing << ",";
      ss << "\"mean\": " << escape_float(m_average.emit()) << ",";
      ss << "\"min\": " << escape_float(m_min.emit()) << ",";
      ss << "\"max\": " << escape_float(m_max.emit()) << ",";
      ss << "\"median\": " << escape_float(median) << ",";
      ss << "\"stdev\": " << escape_float(m_stdv.emit()) << ",";
      ss << "\"numeric\": [" << data << "],";
      ss << "\"categorical\": []";

      return ss.str();

    }

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
template<typename T>
class histogram : public transformation<gl_sarray, histogram_result<T>> {
  public:
    virtual std::vector<histogram_result<T>> split_input(size_t num_threads) override {
      flexible_type current_min = this->m_transformer->min;
      flexible_type current_max = this->m_transformer->max;
      T current_scale_min = this->m_transformer->scale_min;
      T current_scale_max = this->m_transformer->scale_max;
      std::vector<histogram_result<T>> thread_results(num_threads);
      for (auto& thread_result : thread_results) {
        thread_result.init(this->m_source.dtype(), current_min, current_max, current_scale_min, current_scale_max);
      }
      return thread_results;
    }
    virtual void merge_results(std::vector<histogram_result<T>>& thread_results) override {
      for (auto& thread_result : thread_results) {
        // combine summary stats
        this->m_transformer->m_count.combine(thread_result.m_count);
        this->m_transformer->m_count_distinct.combine(thread_result.m_count_distinct);
        this->m_transformer->m_non_null_count.combine(thread_result.m_non_null_count);
        this->m_transformer->m_average.combine(thread_result.m_average);
        this->m_transformer->m_min.combine(thread_result.m_min);
        this->m_transformer->m_max.combine(thread_result.m_max);
        this->m_transformer->m_stdv.combine(thread_result.m_stdv);

        // need to partial_finalize the quantile sketch before combining
        thread_result.m_median.partial_finalize();
        this->m_transformer->m_median.combine(thread_result.m_median);

        // combine histogram
        flexible_type combined_min = std::min(this->m_transformer->min, thread_result.min);
        flexible_type combined_max = std::max(this->m_transformer->max, thread_result.max);
        this->m_transformer->min = combined_min;
        this->m_transformer->max = combined_max;
        this->m_transformer->rescale(combined_min, combined_max);
        thread_result.rescale(combined_min, combined_max);
        DASSERT_EQ(this->m_transformer->scale_min, thread_result.scale_min);
        DASSERT_EQ(this->m_transformer->scale_max, thread_result.scale_max);
        for (size_t i=0; i<histogram_result<T>::REAL_BINS; i++) {
          this->m_transformer->bins[i] += thread_result.bins[i];
        }
      }
    }
    virtual void init(const gl_sarray& source, size_t batch_size) override {
      transformation<gl_sarray, histogram_result<T>>::init(source, batch_size);
      flex_type_enum dtype = this->m_source.dtype();
      if (dtype != flex_type_enum::INTEGER &&
          dtype != flex_type_enum::FLOAT) {
        log_and_throw("dtype of the provided SArray is not valid for histogram. Only int and float are valid dtypes.");
      }

      size_t input_size = this->m_source.size();
      if (input_size >= 2 &&
          this->m_source[0].get_type() != flex_type_enum::UNDEFINED &&
          this->m_source[1].get_type() != flex_type_enum::UNDEFINED &&
          std::isfinite(this->m_source[0].template to<flex_float>()) &&
          std::isfinite(this->m_source[1].template to<flex_float>())) {
        // start with a sane range for the bins (somewhere near the data)
        // (it can be exceptionally small, since the doubling used in resize()
        // will make it converge to the real range quickly)
        this->m_transformer->init(dtype, this->m_source[0], this->m_source[1]);
      } else if (input_size == 1 &&
                this->m_source[0].get_type() != flex_type_enum::UNDEFINED &&
                std::isfinite(this->m_source[0].template to<flex_float>())) {
        // one value, not so interesting
        this->m_transformer->init(dtype, this->m_source[0], this->m_source[0]);
      } else {
        // no data
        this->m_transformer->init(dtype, 0.0, 0.0);
      }
    }
};

std::shared_ptr<Plot> plot_histogram(
  const gl_sarray& sa, const flexible_type& xlabel, const flexible_type& ylabel, 
  const flexible_type& title);

}} // turi::visualization

#endif // __TC_HISTOGRAM
