/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <limits>
#include <future>
#include <unordered_set>
#include <core/data/flexible_type/flexible_type.hpp>
#include <model_server/lib/api/unity_sketch_interface.hpp>
#include <core/storage/sframe_data/sarray.hpp>
#include <ml/sketches/hyperloglog.hpp>
#include <ml/sketches/countsketch.hpp>
#include <ml/sketches/quantile_sketch.hpp>
#include <ml/sketches/streaming_quantile_sketch.hpp>
#include <core/parallel/atomic.hpp>
#include <core/logging/logger.hpp>
namespace turi {

// forward declarations
class unity_sarray;
class unity_sketch;

namespace sketches {

template <typename T, typename Comparator>
class streaming_quantile_sketch;
template <typename T, typename Comparator>
class quantile_sketch;
template <typename T>
class countsketch;
class space_saving_flextype;
class hyperloglog;

} // sketches


/**
 * Provides a query interface to a collection of statistics about an SArray
 * accumulated via various sketching methods.
 * The unity_sketch object contains a summary of a single SArray (a column of
 * an SFrame). It contains sketched statistics about the Array which can be
 * queried efficiently.
 *
 * The sketch computation is fast and has complexity approximately linear
 * in the length of the Array. After which, all queryable functions in the
 * sketch can be performed nearly instantly.
 *
 * The sketch's contents vary depending on whether it is a numeric array,
 * or a non-numeric (string) array, or list type list vector/dict/recursive
 * If numeric:

 * This is essentially a union among a collection of sketches, depend on value
 * type of SArray, here is what's availble in the sketch for each sarray value type:
 *   numeric type (int, float):
 *    - m_numeric_sketch -- numeric summary like max/min/var/std/mean/quantile
 *    - m_discrete_sketch -- discrete summary like unique items/counts/frequent_items
 *   string type:
 *    - m_discrete_sketch -- discrete summary like unique items/counts/frequent_items
 *   dictionary type:
 *    - m_discrete_sketch -- discrete summary like unique items/counts/frequent_items
 *    - m_dict_key_sketch -- sketch summary of flattened dict keys, it is a
 *      sketch summary of string type where we treat each key as string
 *    - m_dict_value_sketch -- sketch summary of flattened dictionary values.
 *      we infer the type of dictionary value by peek into first 100 rows of
 *      of data and then decide whether or not to use numeric sketch.
 *    - m_element_sub_sketch -- optional. Available only if user explicitly asks for
 *       sketch summary for subset of dictionary keys. The sub sketch type is
 *       of the same type as m_dict_value_sketch
 *   vector(array) type:
 *    - m_discrete_sketch -- discrete summary like unique items/counts/frequent_items
 *    - m_element_sketch -- sketch summary for all values of the vector as if the
 *       values are flattened, and the element sketch is of type float (numeric sketch)
 *    - m_element_sub_sketch -- optional. If user asks for sketch summary for certain
 *       columns in the vector value, then this will be available. It is a collection
 *       of sketches for the corresponding columns and the sketch type is numeric.
 *   list(recursive) type:
 *    - m_discrete_sketch -- discrete summary like unique items/counts/frequent_items
 *    - m_element_sketch -- sketch summary for all values of the vector as if the
 *       values are flattened. The element sketch is of type string. We convert all
 *       list values to string and then do a sketch on it
 *
 * The following information is provided exactly:
 *   - length (\ref size())
 *   - # Missing Values (\ref num_undefined())
 *   - Min (\ref min())
 *   - Max (\ref max())
 *   - Mean (\ref mean())
 *   - Variance (\ref var())
 *
 * And the following information is provided approximately:
 *   - # Unique Values (\ref num_unique())
 *   - Quantiles (\ref get_quantile())
 *   - Frequent Items (\ref frequent_items())
 *   - Frequency Count of any Value (\ref frequency_count())

 * For SArray of type recursive/dict/array, additional sketch information is available:
 *   - element_length_summary() -- sketch summary of element length.

 *  For SArray of type list, there is a sketch summary for all values inside the
 *  list element. Sketch summary flattens all list values and do a sketch summery
 *  over flattened values. Each value in list is casted to string for sketch summary.
 *  The summary can be retrieved by calling:
 *   - element_summary() -- sketch summary of all list elements

 *  For SArray of type array(vector), there is a sketch summary for all values inside
 *  vector element. Sketch summary flattens all vector values and do a sketch summery
 *  over flattened values. The summary can be retrieved by calling:
 *   - element_summary() -- sketch summary of all vector elements

 *  For SArray of type dict, additional sketch summary over the keys and values are
 *  provided. They can be retrieved by calling:
 *   - dict_key_summary() -- sketch summary of all keys in the dictionary
 *   - dict_value_summary() -- sketch summary of all values in the dictionary

 *  For SArray of type dict, user can also pass in a list of dictionary keys to
 *  sketch_summary function, this would cause one sub sketch for each of the key.
 *  For example:
 *       >>> sketch = sa.sketch_summary(sub_sketch_keys=["a", "b"])
 *  Then the sub summary may be retrieved by:
 *       >>> sketch.element_sub_sketch()
 *  Or:
 *       >>> sketch.element_sub_sketch(["key1", "key2"])
 *  for subset of keys
 *
 *  Similarly, for SArray of type vector(array), user can also pass in a list of
 *  integers which is the index into the vector to get sub sketch
 *  For example:
 *       >>> sketch = sa.sketch_summary(sub_sketch_keys=[1,3,5])
 *  Then the sub summary may be retrieved by:
 *       >>> sketch.element_sub_sketch()
 *  Or:
 *       >>> sketch.element_sub_sketch([1,3])
 *  for subset of keys
 *
 **/

class unity_sketch: public unity_sketch_base {
 public:

  static const double SKETCH_COMMIT_INTERVAL;

  inline unity_sketch() { }

  ~unity_sketch();

  /**
   * Generates all the sketch statistics from an input SArray.
   * If background is true, the sketch will be constructed in the background.
   * While the sketch is being constructed in a background thread, queries can
   * be executed on the sketch, but none of the quality guarantees will apply.
   */
  void construct_from_sarray(std::shared_ptr<unity_sarray_base> uarray, bool background = false, const std::vector<flexible_type>& keys = {});

  /**
   * Returns true if the sketch is complete.
   * If the sketch is constructed with background == false, this will always
   * return true. If not the sketch is constructed using a background thread
   * and this will return false until the sketch is ready.
   */
  bool sketch_ready();

  /**
   * Returns the number of elements processed by the sketch is complete.
   * If the sketch is constructed with background == false, this will always
   * return the number of elements of the array. If the sketch is
   * constructed using a background thread this may return a value between 0 and
   * the length of the array.
   */
  size_t num_elements_processed();

  /**
   * Returns a sketched estimate of the value at a particular quantile between
   * 0.0 and 1.0. The quantile is guaranteed to be accurate within 1%:
   * meaning that if you ask for the 0.55 quantile, the returned value is
   * guaranteed to be between the true 0.54 quantile and the true
   * 0.56 quantile. The quantiles are only defined for numeric arrays and
   * this function will throw an exception if called on a sketch constructed
   * for a non-numeric column.
   */
  double get_quantile(double quantile);

  /**
   * Returns a sketched estimate of the number of occurances of a given
   * element. This estimate is based on the count sketch. The element type
   * must be of the same type as the input SArray; throws an exception otherwise.
   */
  double frequency_count(flexible_type value);

  /**
   * Returns a sketched estimate of the most frequent elements in the SArray
   * based on the SpaceSaving sketch. It is only guaranteed that all
   * elements which appear in more than 0.01% (0.0001) rows of the array will
   * appear in the set of returned elements. However, other elements may
   * also appear in the result. The item counts are estimated using the
   * CountSketch.
   */
  std::vector<std::pair<flexible_type, size_t> > frequent_items();

  /**
   * Returns a sketched estimate of the number of unique values in the
   * SArray based on the Hyperloglog sketch.
   */
  double num_unique();

  /**
  ** Returns sketch summary for a given key in dictionary SArray sketch, or a given
  ** index in SArray of vector
  **
  ** \param key is either an index into vector or a key in dictionary
  **/
  std::map<flexible_type, std::shared_ptr<unity_sketch_base>> element_sub_sketch(const std::vector<flexible_type>& keys);

  /**
   * Returns element length sketch summary if the sarray is a list/vector/dict type
   * raises exception otherwise
  **/
  std::shared_ptr<unity_sketch_base> element_length_summary();

  /**
   * For SArray of array/list(recursive) type, returns the sketch summary for the list values
   * the summary only works if element can be converted to string. Elements that cannot
   * be converted to string will be ignored
  **/
  std::shared_ptr<unity_sketch_base> element_summary();

  /**
   * For SArray of dictionary type, returns the sketch summary for the dictionary keys
   * It only counts the keys if the key can be converted to string
  **/
  std::shared_ptr<unity_sketch_base> dict_key_summary();

  /**
   * For SArray of dictionary type, returns the sketch summary for the dictionary values
   * It only counts the values if the value can be converted to float
  **/
  std::shared_ptr<unity_sketch_base> dict_value_summary();

  /**
   * Returns the mean of the values in the sarray. Returns 0 on an empty
   * array. Throws an exception if called on an sarray with non-numeric
   * type.
   */
  inline double mean() {
    if (!m_is_numeric) log_and_throw("Mean value not available for a non-numeric column");
    commit_global_if_out_of_date();
    std::unique_lock<turi::mutex> global_lock(lock);
    return m_numeric_sketch.mean;
  }

  /**
   * Returns the max of the values in the sarray. Returns NaN on an empty
   * array. Throws an exception if called on an sarray with non-numeric
   * type.
   */
  inline double max() {
    if (!m_is_numeric) log_and_throw("Max value not available for a non-numeric column");
    commit_global_if_out_of_date();
    std::unique_lock<turi::mutex> global_lock(lock);
    return m_numeric_sketch.max;
  }

  /**
   * Returns the min of the values in the sarray. Returns NaN on an empty
   * array. Throws an exception if called on an sarray with non-numeric
   * type.
   */
  inline double min() {
    if (!m_is_numeric) log_and_throw("Min value not available for a non-numeric column");
    commit_global_if_out_of_date();
    std::unique_lock<turi::mutex> global_lock(lock);
    return m_numeric_sketch.min;
  }

  /**
   * Returns the sum of the values in the sarray. Returns 0 on an empty
   * array. Throws an exception if called on an sarray with non-numeric
   * type.
   */
  inline double sum() {
    if (!m_is_numeric) log_and_throw("Sum value not available for a non-numeric column");
    commit_global_if_out_of_date();
    std::unique_lock<turi::mutex> global_lock(lock);
    return m_numeric_sketch.sum;
  }
  /**
   * Returns the variance of the values in the sarray. Returns 0 on an empty
   * array. Throws an exception if called on an sarray with non-numeric
   * type.
   */
  inline double var() {
    if (!m_is_numeric) log_and_throw("Sum value not available for a non-numeric column");
    commit_global_if_out_of_date();
    std::unique_lock<turi::mutex> global_lock(lock);
    if (m_numeric_sketch.num_items == 0) return 0.0;
    return m_numeric_sketch.m2/m_numeric_sketch.num_items;
  }

  /**
   * Returns the number of elements in the input SArray.
   */
  inline size_t size() {
    return m_size;
  }

  /**
   * Returns the number of undefined elements in the input SArray.
   */
  inline size_t num_undefined() {
    commit_global_if_out_of_date();
    std::unique_lock<turi::mutex> global_lock(lock);
    return m_undefined_count;
  }

  /**
   * Cancels any ongoing sketch computation.
   */
  void cancel();

 private:

  // formula from
  // http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Incremental_Algorithm
  struct numeric_sketch_struct {
    std::shared_ptr<sketches::streaming_quantile_sketch<double>> quantiles;
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::lowest();
    double sum = 0.0;
    double mean = 0.0;
    size_t num_items = 0;
    double m2;

    void reset();

    void combine(const numeric_sketch_struct& other);

    void accumulate(double dval);

    void finalize();
  };

  struct discrete_sketch_struct {
    std::shared_ptr<sketches::countsketch<flexible_type> > count;
    std::shared_ptr<sketches::space_saving_flextype> frequent;
    std::shared_ptr<sketches::hyperloglog> unique;

    void reset();

    void accumulate(const flexible_type& val);

    void combine(const discrete_sketch_struct& other);
  };

  // lock on the global values
  turi::mutex lock;

  bool m_is_child_sketch = false;
  // for child sketch to remember whether parent sketch is ready
  bool m_sketch_ready = false;

  // The values which are set on construction and never changed
  bool m_is_numeric = false;
  bool m_is_list = false;
  double m_size = 0;
  flex_type_enum m_stored_type;
  flex_type_enum m_dict_value_sketch_type;

  // the sketches
  discrete_sketch_struct m_discrete_sketch;

  // statistics
  numeric_sketch_struct m_numeric_sketch;
  size_t m_undefined_count = 0;
  size_t m_num_elements_processed = 0;

  turi::atomic<size_t> m_rows_processed_by_threads;

  // make thread local version of all the sketches
  struct thr_local_data {
    discrete_sketch_struct discrete_sketch;
    numeric_sketch_struct numeric_sketch;
    size_t undefined_count = 0;
    size_t num_elements_processed = 0;
  };

  std::vector<thr_local_data> m_thrlocal;
  std::vector<turi::mutex> m_thrlocks;

  atomic<bool> m_cancel = false;
  std::future<void> m_background_future;
  turi::timer m_commit_timer;

  // for vector/list/dict type, maintain a separate sketch for element length
  std::shared_ptr<unity_sketch> m_element_len_sketch;
  std::shared_ptr<unity_sketch> m_element_sketch;

  // special sketch for dictionary key and value
  std::shared_ptr<unity_sketch> m_dict_key_sketch;
  std::shared_ptr<unity_sketch> m_dict_value_sketch;

  // for vector/dict, allow user to specify a subset keys/index to grab subsketch
  std::map<flexible_type, std::shared_ptr<unity_sketch>> m_element_sub_sketch;

  /*
   * Resets the global sketches and statistics. This function does not
   * acquire locks. The caller must acquire the global lock if necessary.
   */
  void reset_global_sketches_and_statistics();
  void commit_global_if_out_of_date();

  // create a clone of current sketch object
  unity_sketch(std::shared_ptr<unity_sketch> src, bool sketch_ready) {
    m_sketch_ready = sketch_ready;
    m_is_numeric = src->m_is_numeric;
    m_stored_type = src->m_stored_type;
    m_is_list = src->m_is_list;
    m_dict_value_sketch_type = src->m_dict_value_sketch_type;
    m_size = src->m_size;
    m_numeric_sketch = src->m_numeric_sketch;
    m_discrete_sketch = src->m_discrete_sketch;
    m_undefined_count = src->m_undefined_count;
    m_num_elements_processed = src->m_num_elements_processed;
    m_rows_processed_by_threads = src->m_rows_processed_by_threads;
    m_is_child_sketch = src->m_is_child_sketch;
    DASSERT_TRUE(m_is_child_sketch);
  }

  inline void init(
    unity_sketch* parent,
    flex_type_enum type,
    const std::unordered_set<flexible_type>& keys = std::unordered_set<flexible_type>(),
    std::shared_ptr<sarray<flexible_type>::reader_type> reader = NULL);

  inline void combine_global(std::vector<turi::mutex>& thr_locks);
  inline void accumulate_dict_value(const flexible_type& dict_val, size_t thr, const std::unordered_set<flexible_type>& keys);
  inline void accumulate_vector_value(const flexible_type& vect_val, size_t thr, const std::unordered_set<flexible_type>& keys);
  inline void accumulate_list_value(const flexible_type& rec_val, size_t thr);
  inline void accumulate_one_value(size_t thr, const flexible_type& val, const std::unordered_set<flexible_type>& keys = std::unordered_set<flexible_type>());
  inline void accumulate_discrete_sketch(size_t thr, const flexible_type& val) {
      m_thrlocal[thr].discrete_sketch.accumulate(val);
  }

  inline void accumulate_numeric_sketch(size_t thr, const flexible_type& val) {
    m_thrlocal[thr].numeric_sketch.accumulate((double)val);
  }

  inline void empty_sketch() {
    m_numeric_sketch.mean = 0;
    m_numeric_sketch.min = NAN;
    m_numeric_sketch.max = NAN;
    m_numeric_sketch.sum = 0;
    m_numeric_sketch.m2 = 0;
    m_numeric_sketch.num_items = 0;
  }

  inline void increase_nested_element_count(unity_sketch& nested_sketch, size_t thr, size_t count) {
    nested_sketch.m_thrlocal[thr].num_elements_processed += count;
    nested_sketch.m_rows_processed_by_threads.inc(count);
  }

  inline flex_type_enum infer_dict_value_type(std::shared_ptr<sarray<flexible_type>::reader_type> reader);
};
} // namespace turi
