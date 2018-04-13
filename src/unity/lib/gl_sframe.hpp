/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_GL_SFRAME_HPP
#define TURI_UNITY_GL_SFRAME_HPP
#include <cmath>
#include <memory>
#include <cstddef>
#include <string>
#include <iostream>
#include <map>
#include <flexible_type/flexible_type.hpp>
#include <sframe/group_aggregate_value.hpp>
#include <sframe/sframe_rows.hpp>
#include "gl_sarray.hpp"
namespace turi {
class unity_sarray;
class unity_sframe;
class unity_sframe_base;
class gl_sarray;
class sframe;
class sframe_reader;
class sframe_reader_buffer;

class gl_sframe_range;
class gl_sarray_reference;
class const_gl_sarray_reference;

typedef std::map<std::string, flex_type_enum> str_flex_type_map;
typedef std::map<std::string, flexible_type> csv_parsing_config_map;
typedef std::map<std::string, std::string> string_map;
typedef std::map<std::string, std::shared_ptr<unity_sarray_base>> csv_parsing_errors;

/**
 * \ingroup group_glsdk
 * \brief All the available groupby aggregators aggregators.
 * See \ref gl_sframe::groupby for details.
 */
namespace aggregate {

/**
 * Describing an aggregate operation on a set of columns.
 *
 * An object of groupby_descriptor_type can be constructed
 * using functions such as \ref COUNT, \ref SUM, etc for builtin aggregations,
 * or using \ref make_aggregator for customized aggregators.
 */
struct groupby_descriptor_type {

  // default constructor
  groupby_descriptor_type() {};

  // constructor for builtin operators 
  groupby_descriptor_type(const std::string& builtin_operator_name,
                          const std::vector<std::string>& group_columns);

  // constructor for custom operators 
  groupby_descriptor_type(std::shared_ptr<group_aggregate_value> aggregator,
                          const std::vector<std::string>& group_columns);

  /// columns as input into the aggregator
  std::vector<std::string> m_group_columns;

  /// aggregator
  std::shared_ptr<group_aggregate_value> m_aggregator;
};

/**
 * Create a groupby_descriptor_type of user defined groupby aggregator type T.
 *
 * \param group_columns A vector of column names expected by the groupby aggregator.
 * \param const Args&... Extra argument to construct T
 *
 * \code
 * class my_aggregator : public group_aggregate_value {
 *   // default constructible
 *   my_aggregator();
 *
 *   ...
 * };
 *
 * auto aggregator1 = make_aggregator<my_aggregator>({"col1"});
 *
 * class my_complicated_aggregator : public group_aggregate_value {
 *   // constructor requires extra arguments
 *   my_complicated_aggregator(const std::vector<double>& initial_values);
 *
 *   ...
 * };
 *
 * std::vector<double> initial_values {1,2,3};
 * auto aggregator2 = make_aggregator<my_aggregator>({"col1", "col2"},
 *                                                   initial_values);
 * \endcode
 */
template<typename T, typename... Args>
groupby_descriptor_type make_aggregator(const std::vector<std::string>& group_columns,
                                        const Args&... args){
  static_assert(std::is_base_of<group_aggregate_value, T>::value,
                "T must inherit from group_aggregate_value");
  auto aggregator = std::make_shared<T>(&args...);
  return groupby_descriptor_type(aggregator, group_columns);
};

/**
 * Builtin sum aggregator for groupby
 *
 * Example: Get the sum of the rating column for each user.
 * \code
 * sf.groupby({"user"},
 *            {{"rating_sum",aggregate::SUM("rating")}});
 * \endcode
 *
 * \see gl_sframe::groupby
 */
groupby_descriptor_type SUM(const std::string& col);

/**
 * Builtin max aggregator for groupby.
 *
 * Example: Get the max of the rating column for each user.
 * \code
 * sf.groupby({"user"},
 *            {{"rating_max",aggregate::MAX("rating")}});
 * \endcode
 *
 * \see gl_sframe::groupby
 */
groupby_descriptor_type MAX(const std::string& col);

/**
 * Builtin min aggregator for groupby.
 *
 * Example: Get the min of the rating column for each user.
 * \code
 * sf.groupby({"user"},
 *            {{"rating_min",aggregate::MAX("rating")}});
 * \endcode
 *
 * \see gl_sframe::groupby
 */
groupby_descriptor_type MIN(const std::string& col);

/**
 * Builtin count aggregator for groupby.
 *
 * Example: Get the number of occurences of each user
 * \code
 * sf.groupby({"user"},
 *            {{"rating_count",aggregate::COUNT()}});
 * \endcode
 *
 * \see gl_sframe::groupby
 */
groupby_descriptor_type COUNT();


/**
 * Builtin average aggregator for groupby. 
 *
 * Synonym for \ref aggregate::AVG.
 *
 * Example: Get the average rating of each user.
 * \code
 * sf.groupby({"user"},
 *            {{"rating_avg",aggregate::AVG("rating")}});
 * \endcode
 *
 * \see gl_sframe::groupby
 */
groupby_descriptor_type MEAN(const std::string& col);

/**
 * Builtin average aggregator for groupby. 
 *
 * Synonym for \ref aggregate::MEAN.
 *
 * Example: Get the average rating of each user.
 * \code
 * sf.groupby({"user"},
 *            {{"rating_avg",aggregate::AVG("rating")}});
 * \endcode
 *
 * \see gl_sframe::groupby
 */
groupby_descriptor_type AVG(const std::string& col);


/**
 * Builtin variance aggregator for groupby. 
 *
 * Synonym for \ref aggregate::VARIANCE
 *
 * Example: Get the rating variance of each user.
 * \code 
 * sf.groupby({"user"},
 *            {{"rating_var",aggregate::VAR("rating")}});
 * \endcode
 *
 * \see aggregate::VAR
 * \see aggregate::STD
 * \see gl_sframe::groupby
 */
groupby_descriptor_type VAR(const std::string& col);

/**
 * Builtin variance aggregator for groupby. 
 *
 * Synonym for \ref aggregate::VAR.
 *
 * Example: Get the rating variance of each user.
 * \code 
 * sf.groupby({"user"},
 *            {{"rating_var",aggregate::VARIANCE("rating")}});
 * \endcode
 *
 * \see aggregate::VARIANCE
 * \see aggregate::STD
 * \see gl_sframe::groupby
 */
groupby_descriptor_type VARIANCE(const std::string& col);

/**
 * Builtin standard deviation aggregator for groupby. 
 *
 * Synonym for \ref aggregate::STDV.
 *
 * Example: Get the rating standard deviation of each user.
 * \code 
 * sf.groupby({"user"},
 *            {{"rating_std",aggregate::STD("rating")}});
 * \endcode
 *
 * \see aggregate::STDV
 * \see aggregate::VAR
 * \see gl_sframe::groupby
 */
groupby_descriptor_type STD(const std::string& col);


/**
 * Builtin standard deviation aggregator for groupby. 
 *
 * Synonym for \ref aggregate::STD.
 *
 * Example: Get the rating standard deviation of each user.
 * \code 
 * sf.groupby({"user"},
 *            {{"rating_std",aggregate::STDV("rating")}});
 * \endcode
 *
 * \see turi::aggregate::STD
 * \see turi::aggregate::VAR
 * \see gl_sframe::groupby
 */
groupby_descriptor_type STDV(const std::string& col);

/**
 * Builtin aggregator for groupby which selects one row in the group.
 *
 * Example: Get one rating row from a user.
 * \code
 * sf.groupby({"user"},
 *           {{"rating",aggregate::SELECT_ONE("rating")}});
 * \endcode
 * 
 * If multiple columns are selected, they are guaranteed to come from the
 * same row. for instance:
 * \code
 * sf.groupby({"user"},
 *            {{"rating", aggregate::SELECT_ONE("rating")},
 *             {"item", aggregate::SELECT_ONE("item")}});
 * \endcode
 * 
 * The selected "rating" and "item" value for each user will come from the
 * same row in the \ref gl_sframe.
 */
groupby_descriptor_type SELECT_ONE(const std::string& col);

/**
 * Builtin arg minimum aggregator for groupby.
 *
 * Example: Get the number of unique movies
 * \code
 * sf.groupby("user",
 *            {{"best_movie", aggregate::COUNT_DISTINCT("rating")}});
 * \endcode
 */
groupby_descriptor_type COUNT_DISTINCT(const std::string& col);

///@{
/**
 * Builtin aggregator that combines values from one or two columns in one group
 * into either a dictionary value, list value or array value.
 *
 * For example, to combine values from two columns that belong to one group into
 * one dictionary value:
 * \code
 * sf.groupby({"document"},
 *           {{"word_count", aggregate::CONCAT("word", "count")}});
 * \endcode
 * 
 * To combine values from one column that belong to one group into a list value:
 * \code
 * sf.groupby({"user"},
 *            {{"friends", aggregate::CONCAT("friend")}});
 * \endcode
 */
groupby_descriptor_type CONCAT(const std::string& col);
groupby_descriptor_type CONCAT(const std::string& key, const std::string& value);
///@}

///@{
/**
 * Builtin approximate quantile aggregator for groupby.
 *
 * Accepts as an argument, one or more of a list of quantiles to query.
 *
 * To extract the median:
 * \code
 * sf.groupby({"user"}, 
 *            {{"rating_quantiles", aggregate::QUANTILE("rating", 0.5)}});
 * \endcode
 * 
 * To extract a few quantiles:
 * \code
 * sf.groupby({"user"}, 
 *            {{"rating_quantiles", aggregate::QUANTILE("rating", {0.25,0.5,0.75})}});
 * \endcode
 * 
 * Or equivalently
 * \code
 * sf.groupby({"user"}, 
 *            {{"rating_quantiles", aggregate::QUANTILE("rating", {0.25,0.5,0.75})}});
 * \endcode
 * 
 * The returned quantiles are guaranteed to have 0.5% accuracy. That is to say,
 * if the requested quantile is 0.50, the resultant quantile value may be
 * between 0.495 and 0.505 of the true quantile.
 */
groupby_descriptor_type QUANTILE(const std::string& col, double quantile);
groupby_descriptor_type QUANTILE(const std::string& col, const std::vector<double>& quantiles);
///@}

/**
 * Builtin arg maximum aggregator for groupby.
 *
 * Example: Get the movie with maximum rating per user.
 * \code
 * sf.groupby({"user"},
 *            {{"best_movie", aggregate::ARGMAX("rating","movie")}});
 * \endcode
 * 
 */
groupby_descriptor_type ARGMAX(const std::string& agg, const std::string& out);

/**
 * Builtin arg minimum aggregator for groupby.
 *
 * Example: Get the movie with minimum rating per user.
 * \code
 * sf.groupby("user",
 *            {{"best_movie", aggregate::ARGMIN("rating","movie")}});
 * \endcode
 */
groupby_descriptor_type ARGMIN(const std::string& agg, const std::string& out);

} // aggregate

/**
 * \ingroup group_glsdk
 * A tabular, column-mutable dataframe object that can scale to big data. 
 *
 * The data in \ref gl_sframe is stored column-wise on the Turi Server
 * side, and is stored on persistent storage (e.g. disk) to avoid being
 * constrained by memory size.  Each column in an \ref gl_sframe is a
 * immutable \ref gl_sarray, but \ref gl_sframe objects
 * are mutable in that columns can be added and subtracted with ease.  
 * An \ref gl_sframe essentially acts as an ordered dictionary of \ref
 * gl_sarray objects.  
 * Usage:
 *
 * ### Usage
 *
 * The gl_sframe API is designed to very closely mimic the Python SFrame API
 * and supports much of the Python-like capabilities, but in C++. 
 *
 * Column Creation And Referencing
 * \code
 * gl_sframe sf;
 * sf["a"] = gl_sarray{1,2,3,4,5};
 * gl_sarray a_5_element_sarray{1,1,1,1,1};
 * sf["b"] = a_5_element_sarray;
 * gl_sarray some_other_sarray{2,2,2,2,2};
 * sf["c"] = sf["a"] / sf["b"] + some_other_sarray;
 * \endcode
 *
 * Logical Filter:
 * \code
 * gl_sframe sf{{"a", {1,2,3,4,5}},
 *              {"b", {"1","2","3","4","5"}}}; 
 * gl_sframe t = sf[sf["a"] < 3]
 * // t now has 2 columns. a: [1,2] b:["1","2"]
 * \endcode
 *
 * Python Range Slicing:
 * \code
 * gl_sframe sf{{"a", {1,2,3,4,5}},
 *              {"b", {"1","2","3","4","5"}}}; 
 * t = sf[{0,3}];
 * // t is the first 3 rows of sf
 * \endcode
 *
 * And many others.
 *
 * The gl_sframe can be read \b inefficiently using operator[]
 * \code
 * gl_sframe sf{{"a", {1,2,3,4,5}},
 *              {"b", {"1","2","3","4","5"}}}; 
 * std::vector<flexible_type> val = s[2]; 
 * // val[0] == 3, val[1] == "3"
 * \endcode
 *
 * Or iterated efficiently using the \ref range_iterator
 * \code
 * for (const auto& i: sa.range_iterator()) {
 *   ... 
 * }
 * \endcode
 *
 * Note that using "auto" above is more efficient than using vector<flexible_type>
 * \code
 * for (const std::vector<flexible_type> & i: sa.range_iterator()) {
 * \endcode
 *
 * The range_iterator materializes the SFrame if not already materialized, but
 * \ref materialize_to_callback can be used to read the SFrame without
 * materialization.
 *
 * The gl_sframe can constructed in a variety of means: 
 *   - If the data to be written is already in memory, it can be created
 *     using the 
 *     \ref gl_sframe::gl_sframe(const std::map<std::string, std::vector<flexible_type> >& data) 
 *     "gl_sframe constructor"
 *   - Otherwise, the \ref gl_sframe_writer can be used which provides a simple 
 *     write interface.
 *
 * ### Python Binding
 *
 * When used as an input argument in an SDK function, it permits a Python SFrame
 * to be passed as an argument. When used in an output argument, it will return
 * a Python SFrame.
 *
 * For instance:
 * \code
 * //
 * // Compiled as example.so
 * // 
 * gl_sframe add_ones_column(const gl_sframe& data) {
 *   gl_sframe sf = data;
 *   sf["ones"] = 1;
 *   return sf;
 * }
 * BEGIN_FUNCTION_REGISTRATION
 * REGISTER_FUNCTION(add_ones_column, "data");
 * END_FUNCTION_REGISTRATION
 * \endcode
 *
 * Will allow this to be done in Python:
 * \code{.py}
 * import turicreate as gl
 * import example
 * sa = SFrame({"a":[1,2,3,4,5]})
 * ret = example.add_ones_column(sa)
 * # ret now has two columns. "a":[1,2,3,4,5] and "ones":[1,1,1,1,1]
 * \endcode
 *
 * ### Details
 *
 * The gl_sframe is also lazy evaluated behind the scenes to minimize disk
 * access.  This may have the unfortunate effect of hiding errors until
 * materialization is forced to occur. i.e. it might be some time much later in
 * your code that errors will trigger.
 *
 * However, not all operations are lazy and certain operations will force
 * materialization, and that is a constant target for optimization.
 *
 * If you want to force materialization yourself, use \ref materialize()
 */
class gl_sframe {
 public:
  /// Constructs an empty gl_sframe.
  gl_sframe();
  /// Copy Constructor
  gl_sframe(const gl_sframe&);
  /// Move Constructor
  gl_sframe(gl_sframe&&);

  /**
   * Constructs a gl_sframe from a binary SFrame saved previously with 
   * \ref save().
   *
   * \see save()
   */
  explicit gl_sframe(const std::string& directory);

  void construct_from_sframe_index(const std::string& directory);

  /**
   * Constructs a gl_sframe from a csv file 
   */
  void construct_from_csvs(std::string csv_file, csv_parsing_config_map csv_config,
    str_flex_type_map column_type_hints);

  /// Copy assignment
  gl_sframe& operator=(const gl_sframe&);
  /// Move assignment
  gl_sframe& operator=(gl_sframe&&);

  /**
   * Show a visualization of the SFrame.
   */
  void show(const std::string& path_to_client) const;

  /**
   * Constructs a gl_sframe from an in-memory map of values
   * \code
   * std::vector<flexible_type> a{1,2,3,4,5};
   * std::vector<flexible_type> a_str{"1","2","3","4","5"};
   * std::map<std::string, std::vector<flexible_type>> cols;
   * cols["a"] = a;
   * cols["a_str"] = a_str;
   * gl_sframe sf(cols);
   * \endcode
   *
   * Or, more compactly using C++11 initializer lists:
   * \code
   * gl_sframe sf({{"a", a},{"a_str", a_str}});
   * \endcode
   */
  gl_sframe(const std::map<std::string, std::vector<flexible_type> >& data);

  void construct_from_dataframe(const std::map<std::string, std::vector<flexible_type> >& data);

  /**
   * Constructs a gl_sframe from a collection of gl_sarrays.
   *
   * \code
   * gl_sarray a{1,2,3,4,5};
   * gl_sarray a_str{"1","2","3","4","5"}
   * std::map<std::string, gl_sarray> cols;
   * cols["a"] = a;
   * cols["a_str"] = a_str;
   * gl_sframe sf(cols);
   * \endcode
   *
   * Or, more compactly using C++11 initializer lists:
   * \code
   * gl_sframe sf({{"a", a},{"a_str", a_str}});
   * \endcode
   */
  gl_sframe(const std::map<std::string, gl_sarray>& data);

  /**
   * Constructs a gl_sframe from an initializer list of columns.
   *
   * \code
   * gl_sarray a{1,2,3,4,5};
   * gl_sarray a_str{"1","2","3","4","5"}
   * gl_sframe sf{{"a", a},{"a_str", a_str}};
   * \endcode
   */
  gl_sframe(std::initializer_list<std::pair<std::string, gl_sarray>>);
  
  /// \cond TURI_INTERNAL
  /**
   * Implicit conversion from backend unity_sframe object
   */
  gl_sframe(std::shared_ptr<unity_sframe> sframe);
  /**
   * Implicit conversion from backend unity_sframe_base object
   */
  gl_sframe(std::shared_ptr<unity_sframe_base> sframe);
  /**
   * Implicit conversion from backend sframe object
   */
  gl_sframe(const sframe& sframe);

  /**
   * Implicit conversion to backend sframe object
   */
  operator std::shared_ptr<unity_sframe>() const;
  /**
   * Implicit conversion to backend sframe object
   */
  operator std::shared_ptr<unity_sframe_base>() const;

  /**
   * Conversion to materialized backend sframe object.
   */
  sframe materialize_to_sframe() const;
  /// \endcond


  ///@{
  /**
   * Returns the value at a particular array index; generally inefficient.
   *
   * This returns the value of the array at a particular index. Will raise
   * an exception if the index is out of bounds. This operation is generally
   * inefficient: the range_iterator() is prefered.
   */
  std::vector<flexible_type> operator[](int64_t i);
  std::vector<flexible_type> operator[](int64_t i) const;
  ///@}

  ///@{
  /**
   * Performs a slice Python style.
   *
   * \param slice A list of 2 or 3 values. If 2 values, this is interpreted as 
   * {start, end} indices, with an implicit value of step = 1. 
   * If 3 values, this is interpreted as {start, step, end}.
   * Values at the positions [start, start+step, start+2*start, ...] are returned
   * until end (exclusive) is reached. Negative start and end values are 
   * interpreted as offsets from the end of the array. 
   *
   * Given a gl_sframe
   * \code
   * gl_sarray a{1,2,3,4,5,6,7,8,9,10};
   * gl_sframe sf{{"a", a}}
   * \endcode
   *
   * Slicing a consecutive range:
   * \code
   * auto ret = a[{1,4}];  // start at index 1, end at index 4
   * // ret is a gl_sframe with one column a: [2,3,4]
   * \endcode
   *
   * Slicing a range with a step:
   * \code
   * auto ret = a[{1,2,8}];  // start at index 1, end at index 8 with step size 2
   * // ret is a gl_sframe with one column a: [2,4,6,8]
   * \endcode
   *
   * Using negative indexing:
   * \code
   * auto ret = a[{-3,-1}];  // start at end - 3, end at index end - 1
   * // ret a gl_sframe with one column a: [8,9]
   * \endcode
   */
  gl_sframe operator[](const std::initializer_list<int64_t>& slice);
  gl_sframe operator[](const std::initializer_list<int64_t>& slice) const;
  ///@}

  /**
   * Performs a logical filter.
   *
   * This function performs a logical filter: i.e. it subselects all the
   * elements in this array where the corresponding value in the other array
   * evaluates to true.
   * \code
   * gl_sframe sf{{"a", {1,2,3,4,5}},
   *              {"b", {"1","2","3","4","5"}},
   *              {"c", {1.0,2.0,3.0,4.0,5.0}}};
   * auto ret = sf[sf["a"] > 1 && sf["a"] <= 4]; 
   *
   * // ret is now the sframe with 3 columns:
   * // a: [2,3,4]
   * // b: ["2","3","4"]
   * // c: [2.0,3.0,4.0]
   * \endcode
   */
  gl_sframe operator[](const gl_sarray& logical_filter) const;
  
  friend class const_gl_sarray_reference;
  friend class gl_sarray_reference;

  /**
   * \name Column Indexing
   * \anchor column_indexing
   *
   * \brief Selects a single column of the SFrame.
   *
   * This returns an internal array reference object that can be used exactly
   * like a \ref gl_sarray. The design is quite similar to the reference object
   * used by std::vector<bool> for indexing.
   *
   * For instance:
   * 
   * \code 
   * gl_sframe sf{{"a", {1,2,3,4,5}},
   *              {"b", {"1","2","3","4","5"}},
   *              {"c", {1.0,2.0,3.0,4.0,5.0}}};
   * gl_sarray t = sf["a"]; // takes out column "a"
   * \endcode
   *
   * However, this operator can also be used for modifying existing columns,
   * or creating new columns. For instance:
   *
   * \code 
   * gl_sframe sf{{"a", {1,2,3,4,5}},
   *              {"b", {"1","2","3","4","5"}},
   *              {"c", {1.0,2.0,3.0,4.0,5.0}}};
   * sf["a"] = sf["a"] + 1; // sf["a"] is now {2,3,4,5,6}
   * sf["d"] = sf["c"] - 1; // sf["d"] is now {0.0,1.0,2.0,3.0,4.0}
   * \endcode
   *
   * Entire constant columns can also be created the same way:
   * \code
   * gl_sframe sf{{"a", {1,2,3,4,5}},
   *              {"b", {"1","2","3","4","5"}},
   *              {"c", {1.0,2.0,3.0,4.0,5.0}}};
   * sf["ones"] = 1;
   * \endcode
   *
   * Since the returned object is meant to be a short-lived reference, the
   * following is not permitted:
   * \code 
   * gl_sframe sf{{"a", {1,2,3,4,5}},
   *              {"b", {"1","2","3","4","5"}},
   *              {"c", {1.0,2.0,3.0,4.0,5.0}}};
   * auto a_col = sf["a"];
   * \endcode
   * since "auto" resolves to gl_sarray_reference which is intentionally, not
   * copy-constructible.
   *
   * For functional alternatives, See 
   * \ref replace_add_column, 
   * \ref add_column(const flexible_type&, const std::string&) "add_column",  
   * \ref add_column(const gl_sarray&, const std::string&), "add_column overload". 
   */
  ///@{
  const_gl_sarray_reference operator[](const std::string& column) const;
  gl_sarray_reference operator[](const std::string& column);
  ///@}

  //@{
  /**
   * \name Multi-Column Indexing
   * \anchor multi_column_indexing
   * Subselects a subset of columns returning the an SFrame containing only 
   * those columns.
   * 
   * \code
   * gl_sframe sf{{"a", {1,2,3,4,5}},
   *              {"b", {"1","2","3","4","5"}},
   *              {"c", {1.0,2.0,3.0,4.0,5.0}}};
   * gl_sframe ret = sf[{"a", "b"}]
   * // ret has 2 columns "a" and "b"
   * \endcode
   */
  gl_sframe operator[](const std::vector<std::string>& columns) const;
  gl_sframe operator[](const std::initializer_list<std::string>& columns);
  gl_sframe operator[](const std::initializer_list<std::string>& columns) const;
  //@}

  friend class gl_sframe_range;



  /**
   * Calls a callback function passing each row of the SArray.
   *
   * This does not materialize the array if not necessary.
   *
   * The callback may be called in parallel in which case the argument provides
   * a thread number. The function should return false, but may return
   * true at anytime to quit the iteration process. It may also throw exceptions
   * which will be forwarded to the caller of this function.
   *
   * Each call to the callback passes:
   *  - a thread id,
   *  - a shared_ptr to an sframe_rows object
   * 
   * The sframe_rows object looks like a vector<vector<flexible_type>>.
   * i.e. to look at all the rows, you need to write:
   *
   * \code
   * sf.materalize_to_callback([&](size_t, const std::shared_ptr<sframe_rows>& rows) {
   *   for(const auto& row: *rows) {
   *      // each row looks like an std::vector<flexible_type>
   *      // and can be casted to to a vector<flexible_type> if necessayr
   *   }
   * });
   * \endcode
   *
   * \param callback The callback to call
   * \param nthreads Number of threads. If not specified, #cpus is used
   */
  void materialize_to_callback(
      std::function<bool(size_t, const std::shared_ptr<sframe_rows>&)> callback,
      size_t nthreads = (size_t)(-1));

  /**
   * Returns a one pass range object with begin() and end() iterators.
   *
   * This will materialize the array.
   *
   * See \ref materialize_to_callback for a lazy version.
   *
   * \param start The starting index of the range
   * \param end The ending index of the range
   * 
   * \code
   * // create an SFrame
   * gl_sframe sf{{"a", {1,2,3,4,5}},
   *              {"b", {"1","2","3","4","5"}},
   *              {"c", {1.0,2.0,3.0,4.0,5.0}}};
   *
   * // get a range over the entire frame
   * auto ra = sa.range_iterator();
   * auto iter = ra.begin();
   * while (iter != ra.end()) {
   *   std::vector<flexible_type> val = *iter;
   *   // do something to val
   * }
   * \endcode
   *
   * Or more compactly with C++11 syntax:
   * \code
   * for(const auto& val: sa.range_iterator()) {
   *   std::cout << val[0] << " " << val[1] << " " << val[2] << "\n";
   * }
   * \endcode
   *
   * The range returned only supports one pass. The outcome of a second call to
   * begin() is undefined after any iterator is advanced.
   *
   * When iterating over a gl_sframe with many columns, if only a small number
   * of columns are needed, there is a performance benefit to subselecting just
   * those columns first before iterating.
   *
   * i.e. if I only need columns "a" and "b" from the SFrame above:
   * \code
   *
   * for(const auto& val: sa[{"a","b"}].range_iterator()) {
   *   std::cout << val[0] << " " << val[1] << "\n";
   * }
   * \endcode
   *
   * \see gl_sframe_range 
   */
  gl_sframe_range range_iterator(size_t start=0, size_t end=(size_t)(-1)) const;

  /**
   * Returns the number of rows of the SFrame.
   *
   * This may trigger materialization in situations in which the size of the
   * SFrame is not known. For instance after a logical filter.
   *
   * \see has_size
   */
  virtual size_t size() const;

  /**
   * True if size() == 0.
   */
  bool empty() const;

  /**
   * Returns whether or not the sarray has been materialized.
   *
   * \see materialize
   */
  bool is_materialized() const;

  /**
   * Returns true if the size of the SFrame is known. If it is not known,
   * calling size() may trigger materialization.
   */
  bool has_size() const;

  /**
   *  For a SFrame that is lazily evaluated, force persist this sframe to disk,
   *  committing all lazy evaluated operations.
   *
   *  \see is_materialized
   */
  void materialize();

  /**
   * 
   * Saves the SFrame to file.
   *
   * When format is "binary", the saved SArray will be in a directory
   * named with the `targetfile` parameter. When format is "text" or "csv",
   * it is saved as a single human readable text file.
   *
   * \param filename  A local path or a remote URL.  If format is 'text', it
   * will be saved as a text file. If format is 'binary', a directory will be
   * created at the location which will contain the SArray.
   *
   * \param format Optional. Either "binary", "csv" or "". Defaults to "".
   *     Format in which to save the SFrame. Binary saved SArrays can be
   *     loaded much faster and without any format conversion losses.
   *     If "csv", Each row will be written as a single line in an output text
   *     file. If format is an empty string (default), we will try to infer the
   *     format from filename given. If file name ends with "csv", or
   *     ".csv.gz", then the gl_sframe is saved as "csv" format, otherwise the
   *     gl_sframe is saved as 'binary' format.
   */
  void save(const std::string& path, const std::string& format="") const;


  /**
   * Performs an incomplete save of an existing SFrame into a directory.
   * This saved SFrame may reference SFrames in other locations *in the same
   * filesystem* for certain columns/segments/etc.
   *
   * Does not modify the current sframe.
   */
  void save_reference(const std::string& path) const;

  /**
   * Returns an array of types of each column.
   */
  virtual std::vector<flex_type_enum> column_types() const;

  /**
   * Returns the number of columns of the SFrame.
   */
  virtual size_t num_columns() const;

  /**
   * Returns the columns names of the SFrame.
   */
  virtual std::vector<std::string> column_names() const;

  /**
   * Returns true if the column is present in the sframe, and false
   * otherwise.
   */
  bool contains_column(const std::string& col_name) const;

  /**
   * Returns a gl_sframe which contains the first n rows of this gl_sframe.
   *
   * \param n  The number of rows to fetch.
   */
  gl_sframe head(size_t n) const;

  /**
   * Returns a gl_sframe which contains the last n rows of this gl_sframe.
   *
   * \param n  The number of rows to fetch.
   */
  gl_sframe tail(size_t n) const;

  /**
   * Maps each row of the \ref gl_sframe by a given function to a single value.
   * The result \ref gl_sarray is of type "dtype". "fn" should be a function
   * that returns exactly one value which can be cast into the type specified
   * by "dtype". 
   *  
   * \param fn The function to transform each element. Must return exactly one
   *     value which can be cast into the type specified by "dtype".
   *
   * \param dtype The data type of the new \ref gl_sarray. 
   *
   * Example: 
   * \code
   * gl_sframe sf{{"a", {1,2,3,4,5}},
   *              {"c", {1.0,2.0,3.0,4.0,5.0}}};
   * std::cout << sf.apply([](const sframe_rows::row& x) { 
   *                         return x[0] * x[1];
   *                       }, flex_type_enum::FLOAT);
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: float
   * Rows: 5
   * [1.0, 4.0, 9.0, 16.0, 25.0]
   * \endcode
   * 
   * \see gl_sarray::apply
   */
  gl_sarray apply(std::function<flexible_type(const sframe_rows::row&)> fn,
                  flex_type_enum dtype) const;
  /**
   * Create an \ref gl_sframe which contains a subsample of the current 
   * \ref gl_sframe.
   * 
   * \param fraction The fraction of the rows to fetch. Must be between 0 and 1.
   *     
   * Example: 
   * \code
   * gl_sframe sf{{"a", {1,2,3,4,5}},
   *              {"b", {1.0,2.0,3.0,4.0,5.0}}};
   * std::cout <<  sf.sample(.3);
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   *  Columns:
   *      a	integer
   *      b	float
   *  Rows: ?
   *  Data:
   *  +----------------+----------------+
   *  | a              | b              |
   *  +----------------+----------------+
   *  | 4              | 4              |
   *  | 5              | 5              |
   *  +----------------+----------------+
   *  ? rows x 2 columns]
   * \endcode
   */
  gl_sframe sample(double fraction) const;


  /**
   * Create an \ref gl_sframe which contains a subsample of the current 
   * \ref gl_sframe.
   * 
   * \param fraction The fraction of the rows to fetch. Must be between 0 and 1.
   *
   * \param seed The random seed for the random number generator. 
   * Deterministic output is obtained if this is set to a constant.
   *     
   * Example: 
   * \code
   * gl_sframe sf{{"a", {1,2,3,4,5}},
   *              {"b", {1.0,2.0,3.0,4.0,5.0}}};
   * std::cout <<  sf.sample(.3, 12345);
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   *  Columns:
   *      a	integer
   *      b	float
   *  Rows: ?
   *  Data:
   *  +----------------+----------------+
   *  | a              | b              |
   *  +----------------+----------------+
   *  | 4              | 4              |
   *  | 5              | 5              |
   *  +----------------+----------------+
   *  ? rows x 2 columns]
   * \endcode
   */
  gl_sframe sample(double fraction, size_t seed, bool exact=false) const;


  /**
   * Randomly split the rows of an \ref gl_sframe into two \ref gl_sframe
   * objects. The first \ref gl_sframe contains \b M rows, sampled uniformly
   * (without replacement) from the original \ref gl_sframe. \b M is
   * approximately the fraction times the original number of rows. The second
   * \ref gl_sframe contains the remaining rows of the original \ref gl_sframe.
   * 
   * \param fraction Approximate fraction of the rows to fetch for the first returned
   *     \ref gl_sframe. Must be between 0 and 1.
   *     
   * \param seed Optional. Seed for the random number generator used to split.
   *
   * Example: 
   * \code
   * auto sf = gl_sframe({{"id", gl_sarray::from_sequence(0, 1024)}});
   * gl_sframe sf_train, sf_test;
   * std::tie(sf_train, sf_test) = sf.random_split(.95);
   * std::cout <<  sf_test.size() << " " << sf_train.size() << "\n";
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * 102 922
   * \endcode
   */
  std::pair<gl_sframe, gl_sframe> random_split(double fraction) const;

  /**
   * Randomly split the rows of an \ref gl_sframe into two \ref gl_sframe
   * objects. The first \ref gl_sframe contains \b M rows, sampled uniformly
   * (without replacement) from the original \ref gl_sframe. \b M is
   * approximately the fraction times the original number of rows. The second
   * \ref gl_sframe contains the remaining rows of the original \ref gl_sframe.
   * 
   * \param fraction Approximate fraction of the rows to fetch for the first
   *   returned \ref gl_sframe. Must be between 0 and 1.
   *     
   * \param seed The random seed for the random number generator. 
   *    Deterministic output is obtained if this is set to a constant.
   *
   * Example: 
   * \code
   * auto sf = gl_sframe({{"id", gl_sarray::from_sequence(0, 1024)}});
   * gl_sframe sf_train, sf_test;
   * std::tie(sf_train, sf_test) = sf.random_split(.95, 12345);
   * std::cout <<  sf_test.size() << " " << sf_train.size() << "\n";
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * 44 980
   * \endcode
   */
  std::pair<gl_sframe, gl_sframe> random_split(double fraction, size_t seed, bool exact=false) const;

  /**
   * Get top k rows according to the given column. Result is according to and
   * sorted by "column_name" in the given order (default is descending).
   * When "k" is small, "topk" is more efficient than "sort".
   * 
   * \param column_name The column to sort on
   *     
   * \param k Optional. Defaults to 10 The number of rows to return.
   *     
   * \param reverse Optional. Defaults to False. If true, return the top k rows
   *              in ascending order, otherwise, in descending order.
   * 
   * Example: 
   * \code
   * auto sf = gl_sframe({{"id", gl_sarray::from_sequence(1000)}});
   * auto sf["value"] = 0 - sf["id"];
   * std::cout <<  sf.topk("id", k=3);
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +--------+--------+
   * |   id   |  value |
   * +--------+--------+
   * |   999  |  -999  |
   * |   998  |  -998  |
   * |   997  |  -997  |
   * +--------+--------+
   * [3 rows x 2 columns]
   * 
   * \endcode
   * 
   * Example: 
   * \code
   * std::cout << sf.topk("value", k=3);
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +--------+--------+
   * |   id   |  value |
   * +--------+--------+
   * |   1    |  -1    |
   * |   2    |  -2    |
   * |   3    |  -3    |
   * +--------+--------+
   * [3 rows x 2 columns]
   * \endcode
   * 
   * \see sort
   */
  gl_sframe topk(const std::string& column_name, size_t k=10, bool reverse=false) const;

  /**  Returns the index of column `column_name`.
   */
  size_t column_index(const std::string &column_name) const;

  /**  Returns the name of column `index`.
   */
  const std::string& column_name(size_t index) const;


  /**
   * Extracts one column of the gl_sframe.
   *
   * This is equivalent to using \ref column_indexing "operator[]" for column
   * indexing.
   * 
   * Equivalent to:
   * \code
   * sf[colname];
   * \endcode
   * 
   * \see select_columns
   *
   */
  gl_sarray select_column(const std::string& colname) const;

  /**
   * Extracts a collection of columns of the gl_sframe.
   * 
   * This is equivalent to using \ref multi_column_indexing "operator[]" for
   * selecting multiple columns
   * \code
   * sf[colnames];
   * \endcode
   * 
   * \see select_column
   */
  gl_sframe select_columns(const std::vector<std::string>& colnames) const;


  /**
   * Add a column to this \ref gl_sframe, replacing a column with the same name
   * already exists. The number of elements in the data given
   * must match the length of every other column of the \ref gl_sframe. This
   * operation modifies the current \ref gl_sframe in place.
   * If no name is given, a default name is chosen.
   * 
   * \param data The column of data to add.
   *     
   * \param name Optional. The name of the column. If no name is given, a
   *             default name is chosen.
   *
   * This is equivalent to using \ref column_indexing "operator[]" for column
   * assignment.  
   * \code
   * sf[name] = data;
   * \endcode
   * 
   * Example: 
   * \code
   * auto sf = gl_sframe({{"id", {1, 2, 3}},
   *                      {"val", {"A", "B", "C"}}});
   * auto sa = gl_sarray({"cat", "dog", "fossa"});
   * sf.replace_add_column(sa, "species");
   * std::cout <<  sf;
   * \endcode
   *
   * Produces output: 
   * \code{.txt}
   * +----+-----+---------+
   * | id | val | species |
   * +----+-----+---------+
   * | 1  |  A  |   cat   |
   * | 2  |  B  |   dog   |
   * | 3  |  C  |  fossa  |
   * +----+-----+---------+
   * [3 rows x 3 columns]
   * \endcode
   * 
   * \see add_column(const gl_sarray&, const std::string&), 
   */
  virtual void replace_add_column(const gl_sarray& data, const std::string& name="");

  /**
   * Add a column of identical values this \ref gl_sframe, raising an exception
   * if a column the same name already exists. 
   * This operation modifies the current \ref gl_sframe in place.  If no name
   * is given, a default name is chosen.
   * 
   * \param data The value to assign to each entry in the new column
   *     
   * \param name Optional. The name of the column. If no name is given, a
   *             default name is chosen.
   * 
   * This is almost equivalent to using \ref column_indexing "operator[]" for
   * column assignment, but raises an exception if overwriting a column with
   * the same name.
   * \code
   * sf[name] = data;
   * \endcode
   *
   * Example: 
   * \code
   * auto sf = gl_sframe({{"id", {1, 2, 3}},
   *                      {"val", {"A", "B", "C"}}});
   * auto sa = gl_sarray({"cat", "dog", "fossa"});
   * sf.replace_add_column("fish", "species");
   * std::cout <<  sf;
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +----+-----+---------+
   * | id | val | species |
   * +----+-----+---------+
   * | 1  |  A  |  fish   |
   * | 2  |  B  |  fish   |
   * | 3  |  C  |  fish   |
   * +----+-----+---------+
   * [3 rows x 3 columns]
   * \endcode
   * 
   * \see replace_add_column
   * \see add_column(const gl_sarray&, const std::string&)
   */
  virtual void add_column(const flexible_type& data, const std::string& name="");

  /**
   * Add a column to this \ref gl_sframe, raising an exception if a column the
   * same name already exists. The number of elements in the data given
   * must match the length of every other column of the \ref gl_sframe. This
   * operation modifies the current \ref gl_sframe in place.
   * If no name is given, a default name is chosen.
   * 
   * \param data The column of data to add.
   *     
   * \param name Optional. The name of the column. If no name is given, a
   *             default name is chosen.
   * 
   * This is almost equivalent to using \ref column_indexing "operator[]" for
   * column assignment, but raises an exception if overwriting a column with
   * the same name.
   * \code
   * sf[name] = data;
   * \endcode
   *
   * Example: 
   * \code
   * auto sf = gl_sframe({{"id", {1, 2, 3}}, 
   *                      {"val", {"A", "B", "C"}}});
   * auto sa = gl_sarray({"cat", "dog", "fossa"});
   * sf.replace_add_column(sa, "species");
   * std::cout <<  sf;
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +----+-----+---------+
   * | id | val | species |
   * +----+-----+---------+
   * | 1  |  A  |   cat   |
   * | 2  |  B  |   dog   |
   * | 3  |  C  |  fossa  |
   * +----+-----+---------+
   * [3 rows x 3 columns]
   * \endcode
   * 
   * \see replace_add_column
   * \see add_column(const flexible_type&, const std::string&), 
   */
  virtual void add_column(const gl_sarray& data, const std::string& name="");

  /**
   * Adds multiple columns to this \ref gl_sframe. The number of elements in
   * all columns must match the length of every other column of the \ref
   * gl_sframe. This operation modifies the current \ref gl_sframe in place 
   * 
   * \param data The columns to add.
   *     
   * Example: 
   * \code
   * auto sf = gl_sframe({{"id", {1, 2, 3}},
   *                      {"val", {"A", "B", "C"}}});
   * auto sf2 = gl_sframe({{"species", {"cat", "dog", "fossa"}},
   *                       {"age", {3, 5, 9}}});
   * std::cout <<  sf.add_columns(sf2);
   * std::cout <<  sf;
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +----+-----+-----+---------+
   * | id | val | age | species |
   * +----+-----+-----+---------+
   * | 1  |  A  |  3  |   cat   |
   * | 2  |  B  |  5  |   dog   |
   * | 3  |  C  |  9  |  fossa  |
   * +----+-----+-----+---------+
   * [3 rows x 4 columns]
   * \endcode
   * 
   * \see add_column
   */
  virtual void add_columns(const gl_sframe& data);

  /**
   * Remove a column from this \ref gl_sframe. This operation modifies the
   * current \ref gl_sframe in place. Raises an exception if the column
   * does not exist.
   * 
   * \param name The name of the column to remove.
   * 
   * Example: 
   * \code
   * auto sf = gl_sframe({{"id", {1, 2, 3}},
   *                      {"val", {"A", "B", "C"}}});
   * sf.remove("val");
   * std::cout << sf;
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +----+
   * | id |
   * +----+
   * | 1  |
   * | 2  |
   * | 3  |
   * +----+
   * [3 rows x 1 columns]
   * \endcode
   * 
   */
  virtual void remove_column(const std::string& name);

  /**
   * Swap the columns with the given names. This operation modifies the
   * current \ref gl_sframe in place. Raises an exception if the columns do not
   * exist.
   * 
   * \param column_1 Name of column to swap
   *     
   * \param column_2 Name of other column to swap
   * 
   * Example: 
   * \code
   * auto sf = gl_sframe({{"id", {1, 2, 3}},
   *                      { "val", {"A", "B", "C"}}});
   * sf.swap_columns("id", "val");
   * std::cout << sf;
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +-----+-----+
   * | val | id  |
   * +-----+-----+
   * |  A  |  1  |
   * |  B  |  2  |
   * |  C  |  3  |
   * +----+-----+
   * [3 rows x 2 columns]
   * \endcode
   * 
   */
  virtual void swap_columns(const std::string& column_1, const std::string& column_2);


  /**
   * Rename the given columns. "names" is expected to be a dictionary mapping
   * old names to new names. This changes the names of the columns given as
   * the keys and replaces them with the names given as the values.  This
   * operation modifies the current \ref gl_sframe in place.
   * 
   * \param names a map {old-name, new-name} pairs
   * 
   * Example: 
   * \code
   * auto sf = SFrame({{"X1", {"Alice","Bob"}},
   *                   {"X2", {"123 Fake Street","456 Fake Street"}}});
   * sf.rename({{"X1", "name"},{ "X2","address"}});
   * std::cout << sf;
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +-------+-----------------+
   * |  name |     address     |
   * +-------+-----------------+
   * | Alice | 123 Fake Street |
   * |  Bob  | 456 Fake Street |
   * +-------+-----------------+
   * [2 rows x 2 columns]
   * \endcode
   * 
   * \see column_names
   */
  virtual void rename(const std::map<std::string, std::string>& old_to_new_names);


  /**
   * Add the rows of an \ref gl_sframe to the end of this \ref gl_sframe.  Both
   * \ref gl_sframe objects must have the same set of columns with the same
   * column names and column types.
   *
   * \param other Another \ref gl_sframe whose rows are appended to 
   *              the current \ref gl_sframe.
   * 
   * Example: 
   * \code
   * auto sf = gl_sframe({{"id", {4, 6, 8}},
   *                      {"val", {"D", "F", "H"}}});
   * auto sf2 = gl_sframe({{"id", {1, 2, 3}},
   *                       {"val", {"A", "B", "C"}}});
   * auto sf = sf.append(sf2);
   * std::cout <<  sf;
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +----+-----+
   * | id | val |
   * +----+-----+
   * | 4  |  D  |
   * | 6  |  F  |
   * | 8  |  H  |
   * | 1  |  A  |
   * | 2  |  B  |
   * | 3  |  C  |
   * +----+-----+
   * [6 rows x 2 columns]
   * \endcode
   */
  gl_sframe append(const gl_sframe& other) const;

  /**
   * Perform a group on the key_columns followed by aggregations on the columns
   * listed in operations.  The operations parameter is a dictionary that
   * indicates which aggregation operators to use and which columns to use them
   * on. The available operators are SUM, MAX, MIN, COUNT, AVG, VAR, STDV,
   * CONCAT, SELECT_ONE, ARGMIN, ARGMAX, and QUANTILE. For convenience,
   * aggregators MEAN, STD, and VARIANCE are available as synonyms for AVG,
   * STDV, and VAR. See turi::aggregate for more detail on the
   * aggregators.
   *
   * \param groupkeys Columns to group on. Type of key columns can be of any
   *      type other than dictionary.
   *     
   * \param operations Map of columns and aggregation operations. Each key is a
   *     output column name and each value is an aggregator. 
   *
   * Suppose we have an SFrame (sf) with movie ratings by many users.
   * \code{.txt}
   * +---------+----------+--------+
   * | user_id | movie_id | rating |
   * +---------+----------+--------+
   * |  25904  |   1663   |   3    |
   * |  25907  |   1663   |   3    |
   * |  25923  |   1663   |   3    |
   * |  25924  |   1663   |   3    |
   * |  25928  |   1663   |   2    |
   * |  25933  |   1663   |   4    |
   * |  25934  |   1663   |   4    |
   * |  25935  |   1663   |   4    |
   * |  25936  |   1663   |   5    |
   * |  25937  |   1663   |   2    |
   * |   ...   |   ...    |  ...   |
   * +---------+----------+--------+
   * [10000 rows x 3 columns]
   * \endcode
   *
   * Compute the number of occurrences of each user.
   * \code
   * auto user_count = sf.groupby({"user_id"}, 
   *                              {{"count", aggregate::COUNT()}});
   * std::cout << user_count;
   * \endcode
   * \code{.txt}
   * +---------+-------+
   * | user_id | count |
   * +---------+-------+
   * |  62361  |   1   |
   * |  30727  |   1   |
   * |  40111  |   1   |
   * |  50513  |   1   |
   * |  35140  |   1   |
   * |  42352  |   1   |
   * |  29667  |   1   |
   * |  46242  |   1   |
   * |  58310  |   1   |
   * |  64614  |   1   |
   * |   ...   |  ...  |
   * +---------+-------+
   * [9852 rows x 2 columns]
   * \endcode
   *
   * Compute the mean and standard deviation of ratings per user.
   * \code
   * auto  user_rating_stats = sf.groupby({"user_id"}, 
   *                                      {{ "mean_rating", aggregate::MEAN("rating")},
   *                                       {"std_rating", aggregate::STD("rating")}});
   * std::cout << user_rating_stats;
   * \endcode
   * \code{.txt} 
   * +---------+-------------+------------+
   * | user_id | mean_rating | std_rating |
   * +---------+-------------+------------+
   * |  62361  |     5.0     |    0.0     |
   * |  30727  |     4.0     |    0.0     |
   * |  40111  |     2.0     |    0.0     |
   * |  50513  |     4.0     |    0.0     |
   * |  35140  |     4.0     |    0.0     |
   * |  42352  |     5.0     |    0.0     |
   * |  29667  |     4.0     |    0.0     |
   * |  46242  |     5.0     |    0.0     |
   * |  58310  |     2.0     |    0.0     |
   * |  64614  |     2.0     |    0.0     |
   * |   ...   |     ...     |    ...     |
   * +---------+-------------+------------+
   * [9852 rows x 3 columns]
   * \endcode
   *
   * Compute the movie with the minimum rating per user.
   * \code
   * auto chosen_movies = sf.groupby({"user_id"}, 
   *                                 {{ "worst_movies", aggregate::ARGMIN("rating","movie_id")}});
   * std::cout <<  chosen_movies;
   * \endcode
   * \code{.txt}
   * +---------+-------------+
   * | user_id | worst_movies |
   * +---------+-------------+
   * |  62361  |     1663    |
   * |  30727  |     1663    |
   * |  40111  |     1663    |
   * |  50513  |     1663    |
   * |  35140  |     1663    |
   * |  42352  |     1663    |
   * |  29667  |     1663    |
   * |  46242  |     1663    |
   * |  58310  |     1663    |
   * |  64614  |     1663    |
   * |   ...   |     ...     |
   * +---------+-------------+
   * [9852 rows x 2 columns]
   * \endcode
   *
   * Compute the count, mean, and standard deviation of ratings per (user,
   * time), automatically assigning output column names.
   * \code
   * // make up some time column which is a combination of user id and movie id
   * sf["time"] = sf.apply([](const flexible_type& x) {
   *                          return (x[0] + x[1]) % 11 + 2000;
   *                        });
   * auto user_rating_stats = sf.groupby({"user_id", "time"}, 
   *                                     {{"Count", aggregate::COUNT()}, 
   *                                      {"Avg of rating", aggregate::AVG("rating")},
   *                                      {"Stdv of rating", aggregate::STDV("rating")}});
   * std::cout <<  user_rating_stats;
   * \endcode
   * \code{.txt}
   * +------+---------+-------+---------------+----------------+
   * | time | user_id | Count | Avg of rating | Stdv of rating |
   * +------+---------+-------+---------------+----------------+
   * | 2006 |  61285  |   1   |      4.0      |      0.0       |
   * | 2000 |  36078  |   1   |      4.0      |      0.0       |
   * | 2003 |  47158  |   1   |      3.0      |      0.0       |
   * | 2007 |  34446  |   1   |      3.0      |      0.0       |
   * | 2010 |  47990  |   1   |      3.0      |      0.0       |
   * | 2003 |  42120  |   1   |      5.0      |      0.0       |
   * | 2007 |  44940  |   1   |      4.0      |      0.0       |
   * | 2008 |  58240  |   1   |      4.0      |      0.0       |
   * | 2002 |   102   |   1   |      1.0      |      0.0       |
   * | 2009 |  52708  |   1   |      3.0      |      0.0       |
   * | ...  |   ...   |  ...  |      ...      |      ...       |
   * +------+---------+-------+---------------+----------------+
   * [10000 rows x 5 columns]
   * \endcode
   *
   * The groupby function can take a variable length list of aggregation
   * specifiers so if we want the count and the 0.25 and 0.75 quantiles of
   * ratings:
   * \code
   * auto user_rating_stats = sf.groupby({"user_id", "time"}, 
   *                                     {{"Count", aggregate::COUNT()},
   *                                      {"rating_quantiles", agggregate.QUANTILE("rating",{0.25, 0.75}) }});
   * std::cout <<  user_rating_stats;
   * \endcode
   * \code{.txt}
   * +------+---------+-------+------------------------+
   * | time | user_id | Count |    rating_quantiles    |
   * +------+---------+-------+------------------------+
   * | 2006 |  61285  |   1   |      [4.0, 4.0]        |
   * | 2000 |  36078  |   1   |      [4.0, 4.0]        |
   * | 2003 |  47158  |   1   |      [3.0, 3.0]        |
   * | 2007 |  34446  |   1   |      [3.0, 3.0]        |
   * | 2010 |  47990  |   1   |      [3.0, 3.0]        |
   * | 2003 |  42120  |   1   |      [5.0, 5.0]        |
   * | 2007 |  44940  |   1   |      [4.0, 4.0]        |
   * | 2008 |  58240  |   1   |      [4.0, 4.0]        |
   * | 2002 |   102   |   1   |      [1.0, 1.0]        |
   * | 2009 |  52708  |   1   |      [3.0, 3.0]        |
   * | ...  |   ...   |  ...  |          ...           |
   * +------+---------+-------+------------------------+
   * [10000 rows x 4 columns]
   * \endcode
   *
   * To put all items a user rated into one list value by their star rating:
   * \code
   * auto  user_rating_stats = sf.groupby({"user_id", "rating"}, 
   *                                      {{"rated_movie_ids",aggregate::CONCAT("movie_id")}});
   * std::cout <<  user_rating_stats;
   * \endcode
   * \code{.txt}
   * +--------+---------+----------------------+
   * | rating | user_id |     rated_movie_ids  |
   * +--------+---------+----------------------+
   * |   3    |  31434  | array("d", [1663.0]) |
   * |   5    |  25944  | array("d", [1663.0]) |
   * |   4    |  38827  | array("d", [1663.0]) |
   * |   4    |  51437  | array("d", [1663.0]) |
   * |   4    |  42549  | array("d", [1663.0]) |
   * |   4    |  49532  | array("d", [1663.0]) |
   * |   3    |  26124  | array("d", [1663.0]) |
   * |   4    |  46336  | array("d", [1663.0]) |
   * |   4    |  52133  | array("d", [1663.0]) |
   * |   5    |  62361  | array("d", [1663.0]) |
   * |  ...   |   ...   |         ...          |
   * +--------+---------+----------------------+
   * [9952 rows x 3 columns]
   * \endcode
   *
   * To put all items and rating of a given user together into a dictionary
   * value:
   * \code
   * auto  user_rating_stats = sf.groupby({"user_id"}, 
   *                                      {{"movie_rating",agg.CONCAT("movie_id", "rating")}});
   * std::cout <<  user_rating_stats;
   * \endcode
   * \code{.txt}
   * +---------+--------------+
   * | user_id | movie_rating |
   * +---------+--------------+
   * |  62361  |  {1663: 5}   |
   * |  30727  |  {1663: 4}   |
   * |  40111  |  {1663: 2}   |
   * |  50513  |  {1663: 4}   |
   * |  35140  |  {1663: 4}   |
   * |  42352  |  {1663: 5}   |
   * |  29667  |  {1663: 4}   |
   * |  46242  |  {1663: 5}   |
   * |  58310  |  {1663: 2}   |
   * |  64614  |  {1663: 2}   |
   * |   ...   |     ...      |
   * +---------+--------------+
   * [9852 rows x 2 columns]
   * \endcode
   * 
   * \see aggregate
   */
  gl_sframe groupby(const std::vector<std::string>& groupkeys, 
                    const std::map<std::string, aggregate::groupby_descriptor_type>& operators 
                    = std::map<std::string, aggregate::groupby_descriptor_type>()) const;

   /**
    * Joins two \ref gl_sframe objects. Merges the current (left) \ref
    * gl_sframe with the given (right) \ref gl_sframe using a SQL-style
    * equi-join operation by columns.
    * 
    * \param right The \ref gl_sframe to join.
    *     
    * \param on The column name(s) representing the set of join keys.  Each row that
    *     has the same value in this set of columns will be merged together.
    *
    * \param how Optional. The type of join to perform.  "inner" is default.
    *     - \b "inner" : Equivalent to a SQL inner join.  Result consists of the
    *       rows from the two frames whose join key values match exactly,
    *       merged together into one \ref gl_sframe.
    *     - \b "left" : Equivalent to a SQL left outer join. Result is the union
    *       between the result of an inner join and the rest of the rows from
    *       the left \ref gl_sframe, merged with missing values.
    *     - \b "right" : Equivalent to a SQL right outer join.  Result is the union
    *       between the result of an inner join and the rest of the rows from
    *       the right \ref gl_sframe, merged with missing values.
    *     - \b "outer" : Equivalent to a SQL full outer join. Result is
    *       the union between the result of a left outer join and a right
    *       outer join.
    * 
    * Example: 
    * \code
    * auto animals = gl_sframe({{"id", {1, 2, 3, 4}},
    *                           {"name", {"dog", "cat", "sheep", "cow"}}});
    * auto sounds = gl_sframe({{"id", {1, 3, 4, 5}},
    *                          {"sound", {"woof", "baa", "moo", "oink"}}});
    * std::cout <<  animals.join(sounds, {"id"});
    * std::cout <<  animals.join(sounds, {"id"}, "left");
    * std::cout <<  animals.join(sounds, {"id"}, "right");
    * std::cout <<  animals.join(sounds, {"id"}, "outer");
    * \endcode
    * 
    * Produces output: 
    * \code{.txt}
    * +----+-------+-------+
    * | id |  name | sound |
    * +----+-------+-------+
    * | 1  |  dog  |  woof |
    * | 3  | sheep |  baa  |
    * | 4  |  cow  |  moo  |
    * +----+-------+-------+
    * [3 rows x 3 columns]
    * 
    * +----+-------+-------+
    * | id |  name | sound |
    * +----+-------+-------+
    * | 1  |  dog  |  woof |
    * | 3  | sheep |  baa  |
    * | 4  |  cow  |  moo  |
    * | 2  |  cat  |  None |
    * +----+-------+-------+
    * [4 rows x 3 columns]
    * 
    * +----+-------+-------+
    * | id |  name | sound |
    * +----+-------+-------+
    * | 1  |  dog  |  woof |
    * | 3  | sheep |  baa  |
    * | 4  |  cow  |  moo  |
    * | 5  |  None |  oink |
    * +----+-------+-------+
    * [4 rows x 3 columns]
    * 
    * +----+-------+-------+
    * | id |  name | sound |
    * +----+-------+-------+
    * | 1  |  dog  |  woof |
    * | 3  | sheep |  baa  |
    * | 4  |  cow  |  moo  |
    * | 5  |  None |  oink |
    * | 2  |  cat  |  None |
    * +----+-------+-------+
    * [5 rows x 3 columns]
    * \endcode
    */
  gl_sframe join(const gl_sframe& right, 
                 const std::vector<std::string>& joinkeys, 
                 const std::string& how="inner") const;


   /**
    * Joins two \ref gl_sframe objects. Merges the current (left) \ref
    * gl_sframe with the given (right) \ref gl_sframe using a SQL-style
    * equi-join operation by columns.
    * 
    * \param right The \ref gl_sframe to join.
    *     
    * \param on The column name(s) representing a map of join keys from left
    * to right. Each key is taken as a column name on the left gl_sframe
    * and each value is taken as the column name in the right gl_sframe.
    *
    * \param how Optional. The type of join to perform.  "inner" is default.
    *     - \b "inner" : Equivalent to a SQL inner join.  Result consists of the
    *       rows from the two frames whose join key values match exactly,
    *       merged together into one \ref gl_sframe.
    *     - \b "left" : Equivalent to a SQL left outer join. Result is the union
    *       between the result of an inner join and the rest of the rows from
    *       the left \ref gl_sframe, merged with missing values.
    *     - \b "right" : Equivalent to a SQL right outer join.  Result is the union
    *       between the result of an inner join and the rest of the rows from
    *       the right \ref gl_sframe, merged with missing values.
    *     - \b "outer" : Equivalent to a SQL full outer join. Result is
    *       the union between the result of a left outer join and a right
    *       outer join.
    * 
    * Example: 
    * \code
    * auto animals = gl_sframe({{"id", {1, 2, 3, 4}},
    *                           {"name", {"dog", "cat", "sheep", "cow"}}});
    * auto sounds = gl_sframe({{"id", {1, 3, 4, 5}},
    *                          {"sound", {"woof", "baa", "moo", "oink"}}});
    * std::cout <<  animals.join(sounds, {"id", "id"});
    * \endcode
    * 
    * Produces output: 
    * \code{.txt}
    * +----+-------+-------+
    * | id |  name | sound |
    * +----+-------+-------+
    * | 1  |  dog  |  woof |
    * | 3  | sheep |  baa  |
    * | 4  |  cow  |  moo  |
    * +----+-------+-------+
    * [3 rows x 3 columns]
    * \endcode
    */
  gl_sframe join(const gl_sframe& right, 
                 const std::map<std::string, std::string>& joinkeys, 
                 const std::string& how="inner") const;

  /**
   * Filter an \ref gl_sframe by values inside an iterable object. Result is an
   * \ref gl_sframe that only includes (or excludes) the rows that have a
   * column with the given "column_name" which holds one of the values in the
   * given "values" \ref gl_sarray. 
   * 
   * \param values The values to use to filter the \ref gl_sframe.  The
   * resulting \ref gl_sframe will only include rows that have one of these
   * values in the given column.
   *     
   * \param column_name The column of the \ref gl_sframe to match with the
   * given "values".
   *     
   * \param exclude Optional. Defaults to false. If true, the result \ref
   * gl_sframe will contain all rows except those that have one of "values" in
   * "column_name".
   * 
   * Example: 
   * \code
   * auto sf = gl_sframe({{"id", {1, 2, 3, 4}},
   *                      {"animal_type", {"dog", "cat", "cow", "horse"}},
   *                      {"name", {"bob", "jim", "jimbob", "bobjim"}}});
   * auto household_pets = {"cat", "hamster", "dog", "fish", "bird", "snake"};
   * std::cout << sf.filter_by(household_pets, "animal_type");
   * std::cout << sf.filter_by(household_pets, "animal_type", exclude=True);
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +-------------+----+------+
   * | animal_type | id | name |
   * +-------------+----+------+
   * |     dog     | 1  | bob  |
   * |     cat     | 2  | jim  |
   * +-------------+----+------+
   * [2 rows x 3 columns]
   * +-------------+----+--------+
   * | animal_type | id |  name  |
   * +-------------+----+--------+
   * |    horse    | 4  | bobjim |
   * |     cow     | 3  | jimbob |
   * +-------------+----+--------+
   * [2 rows x 3 columns]
   * \endcode
   */
  gl_sframe filter_by(const gl_sarray& values, const std::string& column_name, bool exclude=false) const;

  /**
   * \overload 
   * Pack two or more columns of the current \ref gl_sframe into one single
   * column. The result is a new \ref gl_sframe with the unaffected columns
   * from the original \ref gl_sframe plus the newly created column.
   *
   * The type of the resulting column is decided by the "dtype" parameter.
   * Allowed values for "dtype" are flex_type_enum::DICT ,
   * flex_type_enum::VECTOR or flex_type_enum::LIST 
   * 
   *  - \ref flex_type_enum::DICT : pack to a dictionary \ref gl_sarray where column name becomes
   *    dictionary key and column value becomes dictionary value
   * 
   *  - \ref flex_type_enum::VECTOR : pack all values from the packing columns into an array
   * 
   *  - \ref flex_type_enum::LIST : pack all values from the packing columns into a list.
   *
   * \param columns A list of column names to be packed.  There must 
   * at least two columns to pack. 
   *
   * \param new_column_name Packed column name.  
   *
   * \param dtype Optional. The resulting packed column type. 
   * If not provided, dtype is list.
   *
   * \param fill_na Optional. Value to fill into packed column if missing value
   * is encountered.  If packing to dictionary, "fill_na" is only applicable to
   * dictionary values; missing keys are not replaced.
   * 
   * Example: 
   * Suppose 'sf' is an an SFrame that maintains business category information.
   * \code{.cpp}
   * auto sf = gl_sframe({{"business", {1,2,3,4}},
   *                      {"category.retail", {1, FLEX_UNDEFINED, 1, FLEX_UNDEFINED}},
   *                      {"category.food", {1, 1, FLEX_UNDEFINED, FLEX_UNDEFINED}},
   *                      {"category.service", {FLEX_UNDEFINED, 1, 1, FLEX_UNDEFINED}},
   *                      {"category.shop", {1, 1, FLEX_UNDEFINED, 1}}});
   * std::cout <<  sf;
   * \endcode
   * \code{.txt}
   * +----------+-----------------+---------------+------------------+---------------+
   * | business | category.retail | category.food | category.service | category.shop |
   * +----------+-----------------+---------------+------------------+---------------+
   * |    1     |        1        |       1       |       None       |       1       |
   * |    2     |       None      |       1       |        1         |       1       |
   * |    3     |        1        |      None     |        1         |      None     |
   * |    4     |       None      |       1       |       None       |       1       |
   * +----------+-----------------+---------------+------------------+---------------+
   * [4 rows x 5 columns]
   * \endcode
   *
   * To pack all category columns into a list:
   * \code{.cpp}
   * std::cout <<  sf.pack_columns({"category.retail", "category.food", 
   *                                "category.service", "category.shop"}, 
   *                                "category");
   * \endcode
   * \code{.txt}
   * +----------+--------------------+
   * | business |      category      |
   * +----------+--------------------+
   * |    1     |  [1, 1, None, 1]   |
   * |    2     |  [None, 1, 1, 1]   |
   * |    3     | [1, None, 1, None] |
   * |    4     | [None, 1, None, 1] |
   * +----------+--------------------+
   * [4 rows x 2 columns]
   * \endcode
   *
   * To pack all category columns into a dictionary:
   * \code{.cpp}
   * std::cout << sf.pack_columns({"category.retail", "category.food", 
   *                               "category.service", "category.shop"}, 
   *                               "category", 
   *                               flex_type_enum::DICT);
   * 
   * \endcode
   * \code{.txt}
   * +----------+--------------------------------+
   * | business |               X2               |
   * +----------+--------------------------------+
   * |    1     | {'category.retail': 1, 'ca ... |
   * |    2     | {'category.food': 1, 'cate ... |
   * |    3     | {'category.retail': 1, 'ca ... |
   * |    4     | {'category.food': 1, 'cate ... |
   * +----------+--------------------------------+
   * [4 rows x 2 columns]
   * \endcode
   * \see gl_sframe::unpack
   */
  gl_sframe pack_columns(const std::vector<std::string>& columns,
                         const std::string& new_column_name,
                         flex_type_enum dtype = flex_type_enum::LIST,
                         flexible_type fill_na = FLEX_UNDEFINED) const;
/**
   * Pack two or more columns of the current \ref gl_sframe with a common
   * column name prefix into one single column. The result is a new \ref
   * gl_sframe with the unaffected columns from the original \ref gl_sframe
   * plus the newly created column.
   *
   * The type of the resulting column is decided by the "dtype" parameter.
   * Allowed values for "dtype" are flex_type_enum::DICT ,
   * flex_type_enum::VECTOR or flex_type_enum::LIST 
   * 
   *  - \ref flex_type_enum::DICT : pack to a dictionary \ref gl_sarray where column name becomes
   *    dictionary key and column value becomes dictionary value
   * 
   *  - \ref flex_type_enum::VECTOR : pack all values from the packing columns into an array
   * 
   *  - \ref flex_type_enum::LIST : pack all values from the packing columns into a list.
   *
   * \param column_prefix Packs all columns with the given prefix.
   *
   * \param new_column_name Packed column name.  
   *
   * \param dtype Optional. The resulting packed column type. 
   * If not provided, dtype is list.
   *
   * \param fill_na Optional. Value to fill into packed column if missing value
   * is encountered.  If packing to dictionary, "fill_na" is only applicable to
   * dictionary values; missing keys are not replaced.
   * 
   * Example: 
   * Suppose 'sf' is an an SFrame that maintains business category information.
   * \code{.cpp}
   * auto sf = gl_sframe({{"business", {1,2,3,4}},
   *                      {"category.retail", {1, FLEX_UNDEFINED, 1, FLEX_UNDEFINED}},
   *                      {"category.food", {1, 1, FLEX_UNDEFINED, FLEX_UNDEFINED}},
   *                      {"category.service", {FLEX_UNDEFINED, 1, 1, FLEX_UNDEFINED}},
   *                      {"category.shop", {1, 1, FLEX_UNDEFINED, 1}}});
   * std::cout <<  sf;
   * \endcode
   * \code{.txt}
   * +----------+-----------------+---------------+------------------+---------------+
   * | business | category.retail | category.food | category.service | category.shop |
   * +----------+-----------------+---------------+------------------+---------------+
   * |    1     |        1        |       1       |       None       |       1       |
   * |    2     |       None      |       1       |        1         |       1       |
   * |    3     |        1        |      None     |        1         |      None     |
   * |    4     |       None      |       1       |       None       |       1       |
   * +----------+-----------------+---------------+------------------+---------------+
   * [4 rows x 5 columns]
   * \endcode
   *
   * To pack all category columns into a list:
   * \code{.cpp}
   * std::cout <<  sf.pack_columns("category", "category");
   * \endcode
   * \code{.txt}
   * +----------------+----------------+
   * | business       | category       |
   * +----------------+----------------+
   * | 1              | [1,1,,1]       |
   * | 2              | [,1,1,1]       |
   * | 3              | [1,,1,]        |
   * | 4              | [,,,1]         |
   * +----------------+----------------+
   * [4 rows x 2 columns]
   * \endcode
   *
   * To pack all category columns into a dictionary:
   * \code{.cpp}
   * std::cout << sf.pack_columns("category", 
   *                              "category", 
   *                              flex_type_enum::DICT);
   * 
   * \endcode
   * \code{.txt}
   * +----------+--------------------------------+
   * | business |               X2               |
   * +----------+--------------------------------+
   * |    1     | {'category.retail': 1, 'ca ... |
   * |    2     | {'category.food': 1, 'cate ... |
   * |    3     | {'category.retail': 1, 'ca ... |
   * |    4     | {'category.food': 1, 'cate ... |
   * +----------+--------------------------------+
   * [4 rows x 2 columns]
   * \endcode
   *
   * \see gl_sframe::unpack
   */
  gl_sframe pack_columns(const std::string& column_prefix,
                         const std::string& new_column_name,
                         flex_type_enum dtype = flex_type_enum::LIST,
                         flexible_type fill_na = FLEX_UNDEFINED) const;

   /**
    * Splits a datetime column of \ref gl_sframe to multiple columns, with each
    * value in a separate column. Returns a new \ref gl_sframe with the
    * column replaced with a list of new columns. The expanded column
    * must be of datetime type.  For more details regarding name generation and
    * other, refer to \ref gl_sarray::split_datetime
    *
    * This function is a convenience function which is equivalent to calling 
    * \ref gl_sarray::split_datetime on the column, deleting the column and
    * adding the expanded columns back to the sframe.
    *
    * \param expand_column Name of the column to expand.
    *     
    * \param column_name_prefix Optional. If provided, expanded column names
    * would start with the given prefix.  If not provided, the default value is
    * the name of the expanded column.
    *     
    * \param limit Optional. Limits the set of datetime elements to expand.
    *     Elements are 'year','month','day','hour','minute',
    *     and 'second'.
    *     
    * \param tzone Optional. A boolean parameter that determines whether to
    * show the timezone column or not. Defaults to false.
    * 
    * Example: 
    * \code{.cpp}
    * auto sa = gl_sarray({"20-Oct-2011", "10-Jan-2012"});
    * gl_sframe sf;
    * sf["date"] = sa.str_to_datetime("%d-%b-%Y");
    * auto split_sf = sf.split_datetime("date", "", {"day","year"});
    * std::cout << split_sf;
    * \endcode
    * 
    * Produces output: 
    * \code{.txt}
    *  Columns:
    *      day	integer
    *      year	integer
    *  +----------------+----------------+
    *  | day            | year           |
    *  +----------------+----------------+
    *  | 20             | 2011           |
    *  | 10             | 2012           |
    *  +----------------+----------------+
    *  [2 rows x 2 columns]
    * \endcode
    */
  gl_sframe split_datetime(const std::string& expand_column,
                           const std::string& column_name_prefix = "X", 
                           const std::vector<std::string>& limit = std::vector<std::string>(),
                           bool tzone=false) const;

  /**
   * Expand one column of this \ref gl_sframe to multiple columns with each value in
   * a separate column. Returns a new \ref gl_sframe with the unpacked column
   * replaced with a list of new columns.  The column must be of
   * list/array/dict type.
   * For more details regarding name generation, missing value handling and
   * other, refer to \ref gl_sarray::unpack
   *
   * \param unpack_column Name of the unpacked column
   *     
   * \param column_name_prefix Optional. If provided, unpacked column
   * names would start with the given prefix. Defaults to "X". If the empty
   * string is used, no prefix is used.
   *     
   * \param column_types Optional. Column types for the unpacked columns. If
   * not provided, column types are automatically inferred from first 100 rows.
   * Defaults to FLEX_UNDEFINED.
   *     
   * \param na_value Optional. Convert all values that are equal to "na_value"
   * to missing value if specified.
   *     
   * \param limit  optional limits in the set of list/vector/dict keys to unpack.
   *     For list/vector gl_sarrays, "limit" must contain integer indices.
   *     For dict gl_sarrays, "limit" must contain dictionary keys.
   * 
   * Example: 
   * \code{.cpp}
   * sf = gl_sframe({{"id", {1,2,3}},
   *                 {"wc": {flex_dict{{"a", 1}}, 
   *                         flex_dict{{"b", 2}}, 
   *                         flex_dict{{"a", 1},{"b", 2}}
   *                         }
   *                 }});
   * std::cout << sf;
   * \endcode
   * \code{.txt}
   * +----+------------------+
   * | id |        wc        |
   * +----+------------------+
   * | 1  |     {'a': 1}     |
   * | 2  |     {'b': 2}     |
   * | 3  | {'a': 1, 'b': 2} |
   * +----+------------------+
   * [3 rows x 2 columns]
   * \endcode
   *
   * To unpack:
   * \code{.cpp}
   * std::cout << sf.unpack("wc");
   * \endcode
   * \code{.txt}
   * +----+------+------+
   * | id | wc.a | wc.b |
   * +----+------+------+
   * | 1  |  1   | None |
   * | 2  | None |  2   |
   * | 3  |  1   |  2   |
   * +----+------+------+
   * [3 rows x 3 columns]
   * \endcode
   *
   * To not have prefix in the generated column name::
   * \code{.cpp}
   * std::cout << sf.unpack("wc", "");
   * \endcode
   * \code{.txt}
   * +----+------+------+
   * | id | wc.a | wc.b |
   * +----+------+------+
   * | 1  |  1   | None |
   * | 2  | None |  2   |
   * | 3  |  1   |  2   |
   * +----+------+------+
   * [3 rows x 3 columns]
   * \endcode
   *
   * To limit subset of keys to unpack:
   * \code{.cpp}
   * std::cout << sf.unpack("wc", "", {}, FLEX_UNDEFINED, {"b"});
   * \endcode
   * \code{.txt} 
   * +----+------+
   * | id |   b  |
   * +----+------+
   * | 1  | None |
   * | 2  |  2   |
   * | 3  |  2   |
   * +----+------+
   * [3 rows x 3 columns]
   * \endcode
   * 
   * \see gl_sframe::pack_columns
   * \see gl_sarray::unpack
   */
  gl_sframe unpack(const std::string& unpack_column,
                   const std::string& column_name_prefix = "X", 
                   const std::vector<flex_type_enum>& column_types = std::vector<flex_type_enum>(),
                   const flexible_type& na_value = FLEX_UNDEFINED, 
                   const std::vector<flexible_type>& limit = std::vector<flexible_type>()) const;

/**
 * Convert a "wide" column of an \ref gl_sframe to one or two "tall" columns by
 * stacking all values.
 *
 * The stack works only for columns of list, or array type (for the dict type,
 * see the 
 * \ref stack(const std::string&, const std::vector<std::string>&, bool)const
 * "overload"). One new column is created as a result of stacking, where each
 * row holds one element of the array or list value, and the rest columns
 * from the same original row repeated.
 * 
 * The new \ref gl_sframe includes the newly created column and all columns other
 * than the one that is stacked.
 *
 * \param column_names The column(s) to stack. This column must be of
 * dict/list/array type
 *     
 * \param new_column_name Optional. The new column name. 
 * If not given, column names are generated automatically.
 *     
 * \param drop_na Optional. Defaults to false. If true, missing values and
 * empty list/array/dict are all dropped from the resulting column(s). If
 * false, missing values are maintained in stacked column(s).
 * 
 * Suppose 'sf' is an SFrame that contains a user and his/her friends,
 * where 'friends' columns is an array type. Stack on 'friends' column
 * would create a user/friend list for each user/friend pair:
 * \code
 * auto  sf = gl_sframe({{"topic",{1,2,3}},
 *                       {"friends",{{2,3,4}, {5,6}, {4,5,10,FLEX_UNDEFINED}}}
 *                      });
 * std::cout <<  sf;
 * std::cout <<  sf.stack("friends", "friend");
 * \endcode
 * 
 * Produces output: 
 * \code{.txt}
 * +------+------------------+
 * | user |     friends      |
 * +------+------------------+
 * |  1   |     [2, 3, 4]    |
 * |  2   |      [5, 6]      |
 * |  3   | [4, 5, 10, None] |
 * +------+------------------+
 * [3 rows x 2 columns]
 * 
 * +------+--------+
 * | user | friend |
 * +------+--------+
 * |  1   |  2     |
 * |  1   |  3     |
 * |  1   |  4     |
 * |  2   |  5     |
 * |  2   |  6     |
 * |  3   |  4     |
 * |  3   |  5     |
 * |  3   |  1     |
 * |  3   |  None  |
 * +------+--------+
 * [9 rows x 2 columns]
 * \endcode
 * 
 * \see gl_sframe::unstack(const std::vector<std::string>&, const std::string&) const
 * \see stack(const std::string&, const std::vector<std::string>&, bool)const
 */
  gl_sframe stack(const std::string& column_name,
                  const std::string& new_column_names,
                  bool drop_na = false) const;
/**
 * Convert a "wide" column of an \ref gl_sframe to one or two "tall" columns by
 * stacking all values.
 *
 * The stack works only for columns of dictionary type (for the list or array types,
 * see the 
 * \ref stack(const std::string&, const std::string&, bool)const
 * "overload"). Two new columns are created as a result of
 * stacking: one column holds the key and another column holds the value.
 * The rest of the columns are repeated for each key/value pair.
 *
 * The new \ref gl_sframe includes the newly created columns and all columns
 * other than the one that is stacked.
 *
 * \param column_names The column(s) to stack. This column must be of
 * dict/list/array type
 *     
 * \param new_column_names Optional. The new column names. Must be an vector of 
 * 2 values corresponding to the "key" column and the "value" column.
 * If not given, column names are generated automatically.
 *     
 * \param drop_na Optional. Defaults to false. If true, missing values and
 * empty list/array/dict are all dropped from the resulting column(s). If
 * false, missing values are maintained in stacked column(s).
 * 
 * Suppose 'sf' is an SFrame that contains a column of dict type.
 * Stack would stack all keys in one column and all values in another
 * column:
 * \code
 * auto  sf = gl_sframe({{"topic",{1,2,3,4}}, 
 *                       {"words", {flex_dict{{"a",3},{"cat",2}}, 
 *                                  flex_dict{{"a",1},{"the",2}}, 
 *                                  flex_dict{{"the",1},{"dog",3}}, 
 *                                  flex_dict()}
 *                        }});
 * std::cout <<  sf.stack("words", new_column_name={"word", "count"});
 * \endcode
 * 
 * Produces output: 
 * \code{.txt}
 * +-------+----------------------+
 * | topic |        words         |
 * +-------+----------------------+
 * |   1   |  {'a': 3, 'cat': 2}  |
 * |   2   |  {'a': 1, 'the': 2}  |
 * |   3   | {'the': 1, 'dog': 3} |
 * |   4   |          {}          |
 * +-------+----------------------+
 * [4 rows x 2 columns]
 * 
 * +-------+------+-------+
 * | topic | word | count |
 * +-------+------+-------+
 * |   1   |  a   |   3   |
 * |   1   | cat  |   2   |
 * |   2   |  a   |   1   |
 * |   2   | the  |   2   |
 * |   3   | the  |   1   |
 * |   3   | dog  |   3   |
 * |   4   | None |  None |
 * +-------+------+-------+
 * [7 rows x 3 columns]
 * 
 * Observe that since topic 4 had no words, an empty row is inserted.
 * To drop that row, set dropna=True in the parameters to stack.
 * \endcode
 * 
 * \see unstack(const std::string&, const std::string&) const
 * \see stack(const std::string&, const std::string&, bool)const
 */
  gl_sframe stack(const std::string& column_name,
                  const std::vector<std::string>& new_column_names,
                  bool drop_na = false) const;
 
  /**
   * Concatenate values from one columns into one column, grouping by
   * all other columns. The resulting column could be of type list or array.
   * If "column" is a numeric column, the result will be of vector type.
   * If "column" is a non-numeric column, the new column will be of list type.
   * 
   * \param column The column that is to be concatenated.
   *     If str, then collapsed column type is either array or list.
   *     
   * \param new_column_name Optional. New column name. If not given, a name is
   * generated automatically.
   * 
   * Example: 
   * \code
   * auto  sf = gl_sframe({{"friend", {2, 3, 4, 5, 6, 4, 5, 2, 3}},
   *                       {"user", {1, 1, 1, 2, 2, 2, 3, 4, 4}}});
   * std::cout <<  sf.unstack("friend", "friends");
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +------+-----------------------------+
   * | user |           friends           |
   * +------+-----------------------------+
   * |  3   |            [5.0]            |
   * |  1   |       [2.0, 4.0, 3.0]       |
   * |  2   |       [5.0, 6.0, 4.0]       |
   * |  4   |          [2.0, 3.0]         |
   * +------+-----------------------------+
   * [4 rows x 2 columns]
   * \endcode
   * 
   * \see stack(const std::string&, const std::string&, bool)const
   * \see groupby
   */
  gl_sframe unstack(const std::string& columns,
                    const std::string& new_column_name = "") const;

  /**
   * Concatenate values two columns into one column, grouping by
   * all other columns. The new column will be of dict type where the keys are
   * taken from the first column in the list, and the values taken from the
   * second column in the list.
   * 
   * \param column The columns that are to be concatenated.
   *     
   * \param new_column_name Optional. 
   * New column name. If not given, a name is generated automatically.
   * 
   * Example: 
   * \code
   * auto  sf = gl_sframe({{"count",{4, 2, 1, 1, 2, FLEX_UNDEFINED}},
   *                       {"topic",{"cat", "cat", "dog", "elephant", "elephant", "fish"}},
   *                       {"word", {"a", "c", "c", "a", "b", FLEX_UNDEFINED}}});
   * std::cout <<  sf.unstack({"word", "count"}, "words");
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +----------+------------------+
   * |  topic   |      words       |
   * +----------+------------------+
   * | elephant | {'a': 1, 'b': 2} |
   * |   dog    |     {'c': 1}     |
   * |   cat    | {'a': 4, 'c': 2} |
   * |   fish   |       None       |
   * +----------+------------------+
   * [4 rows x 2 columns]
   * \endcode
   * 
   * \see stack
   * \see groupby
   */
  gl_sframe unstack(const std::vector<std::string>& columns,
                    const std::string& new_column_name = "") const;
 
  /**
   * Remove duplicate rows of the \ref gl_sframe. Will not necessarily preserve the
   * order of the given \ref gl_sframe in the new \ref gl_sframe.
   * 
   * Example:
   * \code
   * gl_sframe sf{ {"id", {1,2,3,3,4}},
   *               {"value", {1,2,3,3,4}} };
   * std::cout << sf.unique() << std::endl;
   * \endcode
   *
   * Produces output:
   * \code{.txt}
   * +----+-------+
   * | id | value |
   * +----+-------+
   * | 2  |   2   |
   * | 4  |   4   |
   * | 3  |   3   |
   * | 1  |   1   |
   * +----+-------+
   * [4 rows x 2 columns]
   * \endcode
   *
   * \see \ref gl_sarray.unique
   */
  gl_sframe unique() const;

  /**
   * Sort current \ref gl_sframe by a single column, using the given sort order.
   *
   * Only columns that are type of str, int and float can be sorted.
   *
   * \param column The name of the column to be sorted.
   *
   * \param ascending Optional. Sort all columns in the given order.
   *
   * Example:
   * \code
   * gl_sframe sf{ {"a", {1,3,2,1}},
   *               {"b", {"a","c","b","b"}},
   *               {"c", {"x","y","z","y"}} };
   * std::cout << sf.sort("a") << std::endl;
   * \endcode
   * 
   * Produces output:
   * \code{.txt}
   * +---+---+---+
   * | a | b | c |
   * +---+---+---+
   * | 1 | a | x |
   * | 1 | b | y |
   * | 2 | b | z |
   * | 3 | c | y |
   * +---+---+---+
   * [4 rows x 3 columns]
   * 
   * \endcode
   * 
   * Example:
   * \code
   * // To sort by column "a", descending
   * std::cout << sf.sort("a", false) << std::endl;
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +---+---+---+
   * | a | b | c |
   * +---+---+---+
   * | 3 | c | y |
   * | 2 | b | z |
   * | 1 | a | x |
   * | 1 | b | y |
   * +---+---+---+
   * [4 rows x 3 columns]
   * \endcode
   * 
   * \see topk
   */
  gl_sframe sort(const std::string& column, bool ascending = true) const;

  /**
   * \overload
   *
   * Sort current \ref gl_sframe by a multiple columns, using the given sort order.
   * 
   * \param columns The names of the columns to be sorted.
   * 
   * \param ascending Optional. Sort all columns in the given order.
   * 
   * The result will be sorted first by
   * first column, followed by second column, and so on. All columns will
   * be sorted in the same order as governed by the "ascending"
   * parameter. 
   * 
   * Example:
   * \code
   * // To sort by column "a" and "b", all ascending
   * std::cout << sf.sort({"a", "b"}) << std::endl;
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +---+---+---+
   * | a | b | c |
   * +---+---+---+
   * | 1 | a | x |
   * | 1 | b | y |
   * | 2 | b | z |
   * | 3 | c | y |
   * +---+---+---+
   * [4 rows x 3 columns]
   * 
   * \endcode
   * 
   * \see topk
   */
  gl_sframe sort(const std::vector<std::string>& columns, bool ascending = true) const;

  /**
   * \overload
   */
  gl_sframe sort(const std::initializer_list<std::string>& columns, bool ascending = true) const;

  /**
   * \overload
   * Sort current \ref gl_sframe by a multiple columns, using different sort order for each column.
   *
   * \param column_and_ascending A map from column name to sort order (ascending is true)
   * 
   * To sort by column "a" ascending, and then by column "c" descending
   * To control the sort ordering for each column
   * individually, "sort_columns" must be a list of (str, bool) pairs.
   * Given this case, the first value is the column name and the second
   * value is a boolean indicating whether the sort order is ascending.
   *
   * Example:
   * \code
   * // To sort by column "a" ascending, and then by column "c" descending
   * std::cout << sf.sort({{"a", true}, {"c", false}}) << std::endl;
   * \endcode
   * 
   * Produces output:
   * \code{.txt}
   * +---+---+---+
   * | a | b | c |
   * +---+---+---+
   * | 1 | b | y |
   * | 1 | a | x |
   * | 2 | b | z |
   * | 3 | c | y |
   * +---+---+---+
   * [4 rows x 3 columns]
   * \endcode
   */
  gl_sframe sort(const std::vector<std::pair<std::string, bool>>& column_and_ascending) const;

  /**
   * Remove missing values from an \ref gl_sframe. A missing value is either "FLEX_UNDEFINED"
   * or "NaN".  If "how" is "any", a row will be removed if any of the
   * columns in the "columns" parameter contains at least one missing
   * value.  If "how" is "all", a row will be removed if all of the columns
   * in the "columns" parameter are missing values.
   * If the "columns" parameter is not specified, the default is to
   * consider all columns when searching for missing values.
   * 
   * \param columns Optional. The columns to use when looking for missing values.
   *    By default, all columns are used.
   *
   * \param how Optional. Specifies whether a row should be dropped if at least one column
   *     has missing values, or if all columns have missing values.  "any" is
   *     default.
   *
   * For instance
   * \code
   * gl_sframe sf { {"a", {1, FLEX_UNDEFINED, FLEX_UNDEFINED}},
   *                {"b", {"a", "b", FLEX_UNDEFINED}} };
   *
   * std::cout << sf.dropna() << std::endl;
   * \endcode
   *
   * Produces output:
   *
   * \code{.txt}
   * +---+---+
   * | a | b |
   * +---+---+
   * | 1 | a |
   * +---+---+
   * [1 rows x 2 columns]
   * \endcode
   *
   * \code
   * // Drop when all values are missing.
   * std::cout << sf.dropna({}, all) << std::endl;
   * \endcode
   *
   * Produces output: 
   * \code{.txt}
   * +------+---+
   * |  a   | b |
   * +------+---+
   * |  1   | a |
   * | None | b |
   * +------+---+
   * [2 rows x 2 columns]
   * 
   * \endcode
   * Example: 
   * \code
   * // Drop rows where column "a" has a missing value.
   * std::cout << sf.dropna({"a"}) << std::endl;
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +---+---+
   * | a | b |
   * +---+---+
   * | 1 | a |
   * +---+---+
   * [1 rows x 2 columns]
   * \endcode
   *
   * \see dropna_split
   */
  gl_sframe dropna(const std::vector<std::string>& columns = std::vector<std::string>(),
                   std::string how = "any") const;

  /**
   * Split rows with missing values from this \ref gl_sframe. This function has
   * the same functionality as dropna, but returns a tuple of two \ref
   * gl_sframe objects.  The first item is the expected output from dropna, and
   * the second item contains all the rows filtered out by the "dropna"
   * algorithm.
   * 
   * \param columns Optional. The columns to use when looking for missing values.
   *    By default, all columns are used.
   * \param how Optional. Specifies whether a row should be dropped if at least
   * one column has missing values, or if all columns have missing values.
   * "any" is default.
   * 
   * Example: 
   * \code
   * gl_sframe sf { {"a": {1, FLEX_UNDEFINED, FLEX_UNDEFINED}},
   *                {"b": {"a", "b", FLEX_UNDEFINED}} };
   * gl_sframe good, bad;
   * std::tie(good, bad) = sf.dropna_split();
   * std::cout << good << std::endl;
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +---+---+
   * | a | b |
   * +---+---+
   * | 1 | a |
   * +---+---+
   * [1 rows x 2 columns]
   * 
   * \endcode
   * 
   * Example: 
   * \code
   * std::cout << bad << std::endl;
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +------+------+
   * |  a   |  b   |
   * +------+------+
   * | None |  b   |
   * | None | None |
   * +------+------+
   * [2 rows x 2 columns]
   * \endcode
   * 
   * \see dropna
   */
  std::pair<gl_sframe, gl_sframe> dropna_split(
      const std::vector<std::string>& columns=std::vector<std::string>(),
      std::string how = "any") const;

  /**
   * Fill all missing values with a given value in a given column. If the
   * "value" is not the same type as the values in "column", this method
   * attempts to convert the value to the original column"s type. If this
   * fails, an error is raised.
   * 
   * \param column The name of the column to modify.
   *
   * \param value The value used to replace all missing values.
   *
   * Example:
   * \code
   * gl_sframe sf {{"a": {1, FLEX_UNDEFINED, FLEX_UNDEFINED},
   *               {"b":["13.1", "17.2", FLEX_UNDEFINED]}};
   * sf = sf.fillna("a", 0);
   * std::cout << sf << std::endl;
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +---+------+
   * | a |  b   |
   * +---+------+
   * | 1 | 13.1 |
   * | 0 | 17.2 |
   * | 0 | None |
   * +---+------+
   * [3 rows x 2 columns]
   * \endcode
   * 
   * \see dropna
   */
  gl_sframe fillna(const std::string& column, flexible_type value) const;

  /**
   * Returns a new \ref gl_sframe with a new column that numbers each row
   * sequentially. By default the count starts at 0, but this can be changed
   * to a positive or negative number.  The new column will be named with
   * the given column name.  An error will be raised if the given column
   * name already exists in the \ref gl_sframe.
   * 
   * \param column_name Optional. The name of the new column that will hold the
   * row numbers.
   *
   * \param start Optional. The number used to start the row number count.
   *
   * Example:
   * \code
   * sf = gl_sframe{{"a": {1, FLEX_UNDEFINED, FLEX_UNDEFINED}},
   *                {"b": {"a", "b", FLEX_UNDEFINED}} };
   * std::cout << sf.add_row_number() << std::endl;
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * +----+------+------+
   * | id |  a   |  b   |
   * +----+------+------+
   * | 0  |  1   |  a   |
   * | 1  | None |  b   |
   * | 2  | None | None |
   * +----+------+------+
   * [3 rows x 3 columns]
   * \endcode
   */
  gl_sframe add_row_number(const std::string& column_name = "id", size_t start = 0) const;

  friend std::ostream& operator<<(std::ostream& out, const gl_sframe& other);

  virtual std::shared_ptr<unity_sframe> get_proxy() const;

 private:
  void instantiate_new();
  void ensure_has_sframe_reader() const;

  std::shared_ptr<unity_sframe> m_sframe;
  mutable std::shared_ptr<sframe_reader> m_sframe_reader;

};


/**
 * Provides printing of the gl_sframe.
 */
std::ostream& operator<<(std::ostream& out, const gl_sframe& other);


/**
 * A range object providing one pass iterators over part or all of a gl_sframe.
 *See \ref gl_sframe::range_iterator for usage examples.
 *
 * \see gl_sframe::range_iterator
 */
class gl_sframe_range {
 public:
  typedef sframe_rows::row type;

  gl_sframe_range(std::shared_ptr<sframe_reader> m_sframe_reader,
                  size_t start, size_t end);
  gl_sframe_range(const gl_sframe_range&) = default;
  gl_sframe_range(gl_sframe_range&&) = default;
  gl_sframe_range& operator=(const gl_sframe_range&) = default;
  gl_sframe_range& operator=(gl_sframe_range&&) = default;

  /// Iterator type
  struct iterator: 
      public boost::iterator_facade<iterator, 
                const sframe_rows::row&, boost::single_pass_traversal_tag> {
   public:
    iterator() = default;
    iterator(const iterator&) = default;
    iterator(iterator&&) = default;
    iterator& operator=(const iterator&) = default;
    iterator& operator=(iterator&&) = default;

    iterator(gl_sframe_range& range, bool is_start);
   private:
    friend class boost::iterator_core_access;
    void increment();
    void advance(size_t n);
    inline bool equal(const iterator& other) const {
      return m_counter == other.m_counter;
    }
    const type& dereference() const;
    size_t m_counter = 0;
    gl_sframe_range* m_owner = NULL;
  };

  /// const_iterator type
  typedef iterator const_iterator;

  /** 
   * Returns an iterator to the start of the range.
   * Once the iterator is advanced, later calls to begin() have undefined
   * behavior.
   *
   * The returned iterator is invalidated once the parent range_iterator is
   * destroyed.
   */
  iterator begin();

  /**
   * Returns an iterator to the end of the range.
   *
   * The returned iterator is invalidated once the parent range_iterator is
   * destroyed.
   */
  iterator end();
 private:  
  std::shared_ptr<sframe_reader_buffer> m_sframe_reader_buffer;
};


/**
 * \ingroup group_glsdk
 * A reference to a column in a gl_sframe.
 * Used to enable 
 * \code
 *  sf["a"] = gl_sarray...
 * \endcode
 */
class gl_sarray_reference: public gl_sarray {
 public:
  gl_sarray_reference() = delete;
  gl_sarray_reference(gl_sarray_reference&&) = default;
  gl_sarray_reference& operator=(gl_sarray_reference&&) = default;
  gl_sarray_reference& operator=(const gl_sarray_reference&);
  gl_sarray_reference& operator=(const gl_sarray&);
  gl_sarray_reference& operator=(const flexible_type& value);
  virtual std::shared_ptr<unity_sarray> get_proxy() const;
 private:
  gl_sarray_reference(const gl_sarray_reference&) = default;

  gl_sarray_reference(gl_sframe& sf, std::string column_name);

  gl_sframe& m_sf;
  std::string m_column_name;
  friend class gl_sframe;
};


/**
 * \ingroup group_glsdk
 * A reference to a column in a gl_sframe.
 * Used to enable 
 * \code
 *  sf["a"] = gl_sarray...
 * \endcode
 */
class const_gl_sarray_reference: public gl_sarray {
 public:
  const_gl_sarray_reference() = delete;
  const_gl_sarray_reference(const_gl_sarray_reference&&) = default;
  virtual std::shared_ptr<unity_sarray> get_proxy() const;
 private:
  const_gl_sarray_reference(const const_gl_sarray_reference&) = default;

  const_gl_sarray_reference(const gl_sframe& sf, std::string column_name);

  const gl_sframe& m_sf;
  std::string m_column_name;
  friend class gl_sframe;
};


class gl_sframe_writer_impl;

/**
 * \ingroup group_glsdk
 * Provides the ability to write \ref gl_sframe.
 * The gl_sframe is internally cut into a collection of segments. Each segment
 * can be written to independently, and the resultant SFrame is the effective
 * concatenation of all the segments.
 *
 * \code
 * // Writes an SFrame of 4 segments, and 2 columns "a" and "b", both of which
 * // are integers.  
 * gl_sframe_writer writer({"a","b"}, 
 *                         {flex_type_enum:INTEGER, flex_type_enum::INTEGER}, 
 *                         4);
 *
 * // for each segment, write a bunch of (i, i) pair values.
 * // segment 0 has 10 0's, 
 * // segment 1 has 10 1's,
 * // etc
 * for (size_t seg = 0;seg < 4; ++seg) {
 *   for (size_t i = 0;i < 10; ++i) {
 *     writer.write({i, i}, seg);
 *   }
 * }
 *
 * gl_sframe sa = writer.close();
 * // sa is now an SFrame of 40 elements comprising of 
 * // four consecutive sequences of (1,1) to (10,10)
 * \endcode
 *
 * Different segments can be written safely in parallel. It is not safe to 
 * write to the same segment simultanously. 
 */
class gl_sframe_writer {
 public:
  /**
   * Constructs a writer to write an gl_sarray of a particular type.
   *
   * \param column_name The column names of the SFrame.
   *
   * \param type The type of each column of the SFrame. Everything written to
   * the writer (via \ref write) must be of that type, is implicitly castable
   * to that type, or is a missing value denoted with a FLEX_UNDEFINED value.
   * 
   * \param num_segments Optional. The number of segments of the SFrame.
   * Adjusting this parameter has little performance impact on the resultant
   * gl_sframe. Modifying this value is only helpful for providing writer
   * parallelism. Defaults to the number of cores on the machine.
   */
  gl_sframe_writer(const std::vector<std::string>& column_names, 
                   const std::vector<flex_type_enum>& column_types, 
                   size_t num_segments = (size_t)(-1));

  /**
   * Writes a single value to a given segment.
   *
   * For instance, 
   * \code
   * gl_sframe_writer({"a","b"}, {flex_type_enum:FLOAT, flex_type_enum::STRING}, 1);
   * writer.write({1.5, "hello"}, 0); 
   * \endcode
   *
   * Different segments can be written safely in parallel. It is not safe to 
   * write to the same segment simultanously. 
   *
   * \param f The value to write. This value should be of an array of the
   * requested typse (as set in the constructor), or is castable to the
   * requested type, or is FLEX_UNDEFINED.
   *
   * \param segmentid The segment to write to.
   */
  void write(const std::vector<flexible_type>& f, size_t segmentid);

  /**
   * Writes a range of values to a given segment.
   *
   * Essentially equivalent to:
   * \code
   * while(start != end) write(*start++);
   * \endcode
   *
   * Different segments can be written safely in parallel. It is not safe to 
   * write to the same segment simultanously. 
   *
   * \param start The start iterator of the range to write.
   *
   * \param end The end iterator of the range to write.
   *
   * \param segmentid The segment to write to.
   */
  template <typename T>
  void write(T begin, T end, size_t segmentid) {
    while (begin != end) {
      write((*begin), segmentid);
      ++begin;
    }
  }

  /**
   * Stops all writes and returns the resultant SFrame.
   */
  gl_sframe close();

  /**
   * Returns the number of segments of the Aarray; this is the same value
   * provided on construction of the writer.
   */
  size_t num_segments() const;

  ~gl_sframe_writer();

 private:
  std::unique_ptr<gl_sframe_writer_impl> m_writer_impl;
};


} // turicreate
#endif // TURI_UNITY_GL_SFRAME_HPP
