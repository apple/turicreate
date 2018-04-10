/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_GL_SARRAY_HPP
#define TURI_UNITY_GL_SARRAY_HPP
#include <cmath>
#include <memory>
#include <cstddef>
#include <string>
#include <iostream>
#include <sframe/sframe_rows.hpp>
#include <sframe/group_aggregate_value.hpp>
#include <flexible_type/flexible_type.hpp>

namespace turi {
/**************************************************************************/
/*                                                                        */
/*                          Forward Declarations                          */
/*                                                                        */
/**************************************************************************/
class unity_sarray;
class unity_sarray_base;
class gl_sframe;
class gl_sarray_range;

template <typename T>
class sarray;

template <typename T>
class sarray_reader;

template <typename T>
class sarray_reader_buffer;

/**
 * \ingroup group_glsdk
 * An immutable, homogeneously typed array object backed by persistent storage.
 *
 * The gl_sarray is a contiguous column of a single type with missing value
 * support, and works with disk to support the holding of data that is much
 * larger than the machine's main memory. Runtime typing of the gl_sarray is
 * managed through the \ref flexible_type, which is an efficient runtime typed
 * value. The types supported by the flexible_type are listed in \ref
 * flex_type_enum. 
 *
 *
 * ### Construction
 *
 * Abstractly the gl_sarray provides an interface to read and write \ref
 * flexible_type values where all values have the same type at runtime (for
 * instance flex_type_enum::INTEGER). The special type \ref
 * flex_type_enum::UNDEFINED (or the value \ref FLEX_UNDEFINED )
 * is used to denote a missing value and can be used in combination with any
 * types.
 *
 * For instance:
 *
 * \code
 * // creates an array of 5 integers
 * gl_sarray g({1,2,3,4,5}); 
 *
 * // creates an array of 5 doubles
 * gl_sarray g({1.0,2.0,3.0,4.0,5.0}); 
 *
 * // creates an array of 4 doubles with one missing value
 * gl_sarray g({1.0,2.0,3.0,FLEX_UNDEFINED,5.0}); 
 * \endcode
 *
 * While the gl_sarray is conceptually immutable, all that really means is that
 * element-wise modifications are not permitted. However, full SArray assignments
 * are permitted.
 *
 * \code
 * gl_sarray g({1,2,3,4,5}); 
 * gl_sarray s = g + 1;
 * // s is {2,3,4,5,6}
 * \endcode
 *
 * ### Usage
 *
 * The gl_sarray API is designed to very closely mimic the Python SArray API
 * and supports much of the Python-like capabilities, but in C++. 
 *
 * For instance, vector and operations:
 * \code
 * gl_sarray s{1,2,3,4,5};
 * gl_sarray y{2.0,3.0,2.5,1.5,2.5};
 * auto t = (s + 10) / y;
 * \endcode
 *
 * Logical filters:
 * \code
 * gl_sarray s{1,2,3,4,5};
 * gl_sarray selector{0,0,1,1,1}
 * auto t = s[selector]; 
 * // t is [3,4,5]
 *
 *
 * gl_sarray s{1,2,3,4,5};
 * auto t = s[s < 3]; 
 * // t is [1,2]
 * \endcode
 *
 * Python Range slicing:
 * \code
 * gl_sarray s{1,2,3,4,5};
 * auto t = s[{0,3}]; 
 * auto u = s[{-3,-1}]; 
 *
 * // t is [1,2]
 * // u is [3,4]
 * \endcode
 *
 * And many others. 
 *
 * The gl_sarray can be read \b inefficiently using operator[]
 * \code
 * gl_sarray s{1,2,3,4,5};
 * int val = s[2]; 
 * // val == 3
 * \endcode
 *
 * Or iterated efficiently using the \ref range_iterator
 * \code
 * for (const auto& i: sa.range_iterator() {
 *   ...
 * }
 * \endcode
 *
 *
 * The range_iterator materializes the SFrame if not already materialized, but
 * \ref materialize_to_callback can be used to read the SFrame without
 * materialization.
 *
 * The gl_sarray can constructed in a variety of means: 
 *   - If the data to be written is already in memory, it can be created
 *     using the 
 *     \ref gl_sarray::gl_sarray(const std::vector<flexible_type>& values, flex_type_enum dtype) "gl_sarray constructor"
 *   - Otherwise, the \ref gl_sarray_writer can be used which provides a simple 
 *     write interface.
 *
 * ### Python Binding
 *
 * When used as an input argument in an SDK function, it permits a Python SArray
 * to be passed as an argument. When used in an output argument, it will return
 * a Python SArray.
 *
 * For instance:
 * \code
 * //
 * // Compiled as example.so
 * // 
 * gl_sarray add_one_to_array(gl_sarray data) {
 *   return s + 1;
 * }
 * BEGIN_FUNCTION_REGISTRATION
 * REGISTER_FUNCTION(add_one_to_array, "data");
 * END_FUNCTION_REGISTRATION
 * \endcode
 *
 * Will allow this to be done in Python:
 * \code{.py}
 * import turicreate as gl
 * import example
 * sa = SArray([1,2,3,4,5])
 * ret = example.add_one_to_array(sa)
 * # ret is now [2,3,4,5,6]
 * \endcode
 *
 * ### Details
 *
 * The gl_sarray is internally a reference object. i.e. in the code below,
 * both a and b will point to the same underlying sarray. However since 
 * gl_sarray's are immutable, this does not introduce any interface quirks.
 * \code
 * gl_sarray a{1,2,3};
 * gl_sarray b = a;
 * \endcode
 * 
 * The gl_sarray is also lazy evaluated behind the scenes to minimize disk 
 * access. Thus regardless of the size of the SArray or the complexity of the 
 * lambda operation, this operation will run quickly.
 * \code
 * b = (sa.apply(some_complicated_function) + 5) / 2;
 * \endcode
 *
 * This may have the unfortunate effect of hiding errors until materialization
 * is forced to occur. i.e. it might be some time much later in your code
 * that errors in some_complicated_function will trigger.
 *
 * However, not all operations are lazy and certain operations will force
 * materialization, and that is a constant target for optimization.
 *
 * If you want to force materialization yourself, use \ref materialize()
 */
class gl_sarray {
 public:
  /// Constructs an empty SArray
  gl_sarray();

  /// Copy Constructor
  gl_sarray(const gl_sarray&);

  /// Move Constructor 
  gl_sarray(gl_sarray&&);

  /// Copy Assignment
  gl_sarray& operator=(const gl_sarray&);

  /// Move Assignment
  gl_sarray& operator=(gl_sarray&&);

  /**
   * Constructs a gl_sarray from a binary SArray saved previously with 
   * \ref save().
   *
   * \see save()
   */
  explicit gl_sarray(const std::string& directory);

  /**
   * Constructs an gl_sarray from a in memory vector of values.
   * \code
   * std::vector<flexible_type> values{1,2,3,4,5};
   *
   * // auto infers data type
   * gl_sarray sa(values);
   *
   * // resultant array is of floating point type. 
   * // Automatic type casting is performed internally.
   * gl_sarray sa(values, flex_type_enum::FLOAT); 
   * \endcode
   *
   */
  gl_sarray(const std::vector<flexible_type>& values, 
            flex_type_enum dtype = flex_type_enum::UNDEFINED);

  void construct_from_vector(const std::vector<flexible_type>& values,
            flex_type_enum dtype = flex_type_enum::UNDEFINED);

  /**
   * Constructs a gl_sarray from an initializer list of values.
   *
   * Type is automatically determined.
   * \code
   * // creates an array of 5 integers
   * gl_sarray g({1,2,3,4,5}); 
   *
   * // creates an array of 5 doubles
   * gl_sarray g({1.0,2.0,3.0,4.0,5.0}); 
   *
   * // non-contiguous type. Most general type is selected. 
   * // This will result in an array of strings.
   * gl_sarray g({1,2.0,"3"}); 
   * \endcode
   */  
  gl_sarray(const std::initializer_list<flexible_type>& values);

/**************************************************************************/
/*                                                                        */
/*                          Static Constructors                           */
/*                                                                        */
/**************************************************************************/

  /**
   * Returns a gl_sarray of size with a constant value.
   *
   * \param value The value to fill the array
   * \param size The size of the array
   *
   * \code
   * // Construct an SArray consisting of 10 zeroes:
   * gl_sarray zeros = gl_sarray::from_const(0, 10);
   * \endcode
   */
  static gl_sarray from_const(const flexible_type& value, size_t size);

  /**
   * Returns a gl_sarray of a sequence of integer values.
   * \param start The starting value
   * \param end One past the last value
   * \param reverse If the values are in reverse
   *
   * \code
   * // returns a sequence of values from 0 to 99
   * gl_sarray::from_sequence(0, 100);
   * // returns a sequence of values from 99 to 0
   * gl_sarray::from_sequence(0, 100, true);
   * \endcode
   */
  static gl_sarray from_sequence(size_t start, size_t end, bool reverse=false);

  /**
  /**************************************************************************/
  /*                                                                        */
  /*                        Implicit Type Converters                        */
  /*                                                                        */
  /**************************************************************************/
  /// \cond TURI_INTERNAL
  /**
   * \internal
   * Implicit conversion from backend unity_sarray objects.
   */ 
  gl_sarray(std::shared_ptr<unity_sarray> sarray);
  /**
   * \internal
   * Implicit conversion from backend unity_sarray_base objects.
   */ 
  gl_sarray(std::shared_ptr<unity_sarray_base> sarray);
  /**
   * \internal
   * Implicit conversion from backend sarray objects.
   */
  gl_sarray(std::shared_ptr<sarray<flexible_type> > sarray);
  /**
   * \internal
   * Implicit conversion to backend sarray objects.
   */ 
  operator std::shared_ptr<unity_sarray>() const;
  /**
   * \internal
   * Implicit conversion to backend sarray objects.
   */ 
  operator std::shared_ptr<unity_sarray_base>() const;

  /**
   * \internal
   * Conversion to materialized backend sarray object.
   */
  std::shared_ptr<sarray<flexible_type> > materialize_to_sarray() const;

  /// \endcond
 
  /**************************************************************************/
  /*                                                                        */
  /*                           Operator Overloads                           */
  /*                                                                        */
  /**************************************************************************/

  /**
   * \name Numeric operator overloads.
   *
   * Most operators are overloaded and will perform element-wise operations
   * on the entire array. 
   *
   * For instance:
   * \code
   * gl_sarray a{1,2,3,4,5};
   * // an array of 5 exclamation marks
   * gl_sarray b = gl_sarray::from_const("!", 5);
   *
   * auto ret = (2 * sa - 1).astype(flex_type_enum::STRING) + b;
   * // results in ret being the array ["1!", "3!", "5!", "7!", "9!"];
   * \endcode
   *
   * Comparison operators will return a gl_sarray of binary integers.
   * \code
   * gl_sarray a{1,2,3,4,5};
   * auto ret = a > 3; 
   * // ret will be an integer array containing [0,0,0,1,1]
   * \endcode
   *
   * Logical and bitwise operators are equivalent: & and && mean the same thing
   * and | and || and provide logical element-wise "and" and "or"s.
   *
   * \code
   * gl_sarray a{1,2,3,4,5};
   * auto ret = a > 1 && a <= 4; 
   * // ret will be an integer array containing [0,1,1,1,0]
   * \endcode
   *
   * These are useful for the logical filter operation:
   * \code
   * gl_sarray a{1,2,3,4,5};
   * gl_sarray b = a.astype(flex_type_enum::STRING);
   * auto ret = b[a > 1 && a <= 4]; 
   * // ret will be an string array containing ["2","3","4"]
   * \endcode
   *
   * The logical and bitwise operators can be used with non-integral arrays 
   * in which case all empty values evaluate to False. i.e. for string,
   * list, and dictionary SArrays, empty values are interpreted as false.
   *
   * For instance:
   * \code
   * gl_sarray a{"1","","2"}; // sarray of strings
   * gl_sarray b{1,1,0}; // sarray of integers
   * auto ret = a && b; // ret is now {1, 0, 0}
   * \endcode
   */
  ///@{
  gl_sarray operator+(const gl_sarray& other) const;
  gl_sarray operator-(const gl_sarray& other) const;
  gl_sarray operator*(const gl_sarray& other) const;
  gl_sarray operator/(const gl_sarray& other) const;
  gl_sarray operator<(const gl_sarray& other) const;
  gl_sarray operator>(const gl_sarray& other) const;
  gl_sarray operator<=(const gl_sarray& other) const;
  gl_sarray operator>=(const gl_sarray& other) const;
  gl_sarray operator==(const gl_sarray& other) const;

  gl_sarray operator+(const flexible_type& other) const;
  gl_sarray operator-(const flexible_type& other) const;
  gl_sarray operator*(const flexible_type& other) const;
  gl_sarray operator/(const flexible_type& other) const;
  gl_sarray operator<(const flexible_type& other) const;
  gl_sarray operator>(const flexible_type& other) const;
  gl_sarray operator<=(const flexible_type& other) const;
  gl_sarray operator>=(const flexible_type& other) const;
  gl_sarray operator==(const flexible_type& other) const;

  gl_sarray operator+=(const gl_sarray& other);
  gl_sarray operator-=(const gl_sarray& other);
  gl_sarray operator*=(const gl_sarray& other);
  gl_sarray operator/=(const gl_sarray& other);

  gl_sarray operator+=(const flexible_type& other);
  gl_sarray operator-=(const flexible_type& other);
  gl_sarray operator*=(const flexible_type& other);
  gl_sarray operator/=(const flexible_type& other);
  gl_sarray operator&&(const gl_sarray& other) const;
  gl_sarray operator||(const gl_sarray& other) const;
  gl_sarray operator&(const gl_sarray& other) const;
  gl_sarray operator|(const gl_sarray& other) const;
  ///@}

  /**
   * Performs an element-wise substring search of "item". The current array
   * must contains strings and item must be a string. Produces a 1 for each
   * row if item is a substring of the row and 0 otherwise.
   */
  gl_sarray contains(const flexible_type& other) const;

  /**
   * Returns the value at a particular array index; generally inefficient.
   *
   * This returns the value of the array at a particular index. Will raise
   * an exception if the index is out of bounds. This operation is generally
   * inefficient: the range_iterator() is prefered.
   */
  flexible_type operator[](int64_t i) const;

  /**
   * Performs a logical filter.
   *
   * This function performs a logical filter: i.e. it subselects all the
   * elements in this array where the corresponding value in the other array
   * evaluates to true.
   * \code
   * gl_sarray a{1,2,3,4,5};
   * auto ret = a[a > 1 && a <= 4]; 
   * // ret is now the array [2,3,4]
   * \endcode
   */
  gl_sarray operator[](const gl_sarray& slice) const;

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
   * Given a gl_sarray
   * \code
   * gl_sarray a{1,2,3,4,5,6,7,8,9,10};
   * \endcode
   *
   * Slicing a consecutive range:
   * \code
   * auto ret = a[{1,4}];  // start at index 1, end at index 4
   * // ret is the array [2,3,4]
   * \endcode
   *
   * Slicing a range with a step:
   * \code
   * auto ret = a[{1,2,8}];  // start at index 1, end at index 8 with step size 2
   * // ret is the array [2,4,6,8]
   * \endcode
   *
   * Using negative indexing:
   * \code
   * auto ret = a[{-3,-1}];  // start at end - 3, end at index end - 1
   * // ret is the array [8,9]
   * \endcode
   */
  gl_sarray operator[](const std::initializer_list<int64_t>& slice) const;

  /**************************************************************************/
  /*                                                                        */
  /*                              Make Friends                              */
  /*                                                                        */
  /**************************************************************************/

  friend gl_sarray operator+(const flexible_type& opnd, const gl_sarray& opnd2);
  friend gl_sarray operator-(const flexible_type& opnd, const gl_sarray& opnd2);
  friend gl_sarray operator*(const flexible_type& opnd, const gl_sarray& opnd2);
  friend gl_sarray operator/(const flexible_type& opnd, const gl_sarray& opnd2);
  friend gl_sarray operator<(const flexible_type& opnd, const gl_sarray& opnd2);
  friend gl_sarray operator>(const flexible_type& opnd, const gl_sarray& opnd2);
  friend gl_sarray operator<=(const flexible_type& opnd, const gl_sarray& opnd2);
  friend gl_sarray operator>=(const flexible_type& opnd, const gl_sarray& opnd2);
  friend gl_sarray operator==(const flexible_type& opnd, const gl_sarray& opnd2);


  /**************************************************************************/
  /*                                                                        */
  /*                               Iterators                                */
  /*                                                                        */
  /**************************************************************************/
  friend class gl_sarray_range;


  /**
   * Calls a callback function passing each row of the SArray.
   *
   * This does not materialize the array if not necessary.
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
   * sa.materalize_to_callback([&](size_t, const std::shared_ptr<sframe_rows>& rows) {
   *   for(const auto& row: *rows) {
   *      // each row looks like an std::vector<flexible_type>
   *      // and can be casted to to a vector<flexible_type> if necessary
   *
   *      // But this this is an sarray, the element you want is always in 
   *      // row[0]
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
   * // create a sequence of 1,000 integer values
   * gl_sarray sa = gl_sarray::from_sequence(0,1000);
   *
   * // get a range over the entire array
   * auto ra = sa.range_iterator();
   * auto iter = ra.begin();
   * while (iter != ra.end()) {
   *   std::cout << *iter;
   *   ++iter;
   * }
   * \endcode
   *
   * Or more compactly with C++11 syntax:
   * \code
   * for(const auto& val: sa.range_iterator()) {
   *   std::cout << val << "\n";
   * }
   * \endcode
   *
   * The range returned only supports one pass. The outcome of a second call to
   * begin() is undefined after any iterator is advanced.
   *
   * \see gl_sarray_range
   */
  gl_sarray_range range_iterator(size_t start=0, size_t end=(size_t)(-1)) const;

  /**************************************************************************/
  /*                                                                        */
  /*                          All other functions                           */
  /*                                                                        */
  /**************************************************************************/

  /**
   * 
   * Saves the gl_sarray to file.
   *
   * When format is "binary" (default), the saved SArray will be in a directory
   * named with the `targetfile` parameter. When format is "text" or "csv",
   * it is saved as a single human readable text file.
   *
   * \param filename  A local path or a remote URL.  If format is 'text', it
   * will be saved as a text file. If format is 'binary', a directory will be
   * created at the location which will contain the SArray.
   *
   * \param format Either "binary", "text", "csv". Defaults to "binary". optional.
   *     Format in which to save the SFrame. Binary saved SArrays can be
   *     loaded much faster and without any format conversion losses.
   *     'text' and 'csv' are synonymous: Each SArray row will be written
   *     as a single line in an output text file. If not
   *     given, will try to infer the format from filename given. If file
   *     name ends with 'csv', 'txt' or '.csv.gz', then save as 'csv' format,
   *     otherwise save as 'binary' format.
   */
  void save(const std::string& directory, const std::string& format="binary") const;

  /**
   * The size of the SArray.
   */
  size_t size() const;

  /**
   * True if size() == 0.
   */
  bool empty() const;

  /**
   * Returns data type of the gl_sarray.
   *
   * \code
   * gl_sarray sa{1,2,3,4,5};
   * flex_type_enum dtype = sa.dtype(); // dtype is flex_type_enum::INTEGER
   *
   * gl_sarray sa{"1","2","3","4","5"};
   * flex_type_enum dtype = sa.dtype(); // dtype is flex_type_enum::STRING
   * \endcode
   */
  flex_type_enum dtype() const;

  /**
   *  For a gl_sarray that is lazily evaluated, force persist this sarray to disk,
   *  committing all lazy evaluated operations.
   *
   *  \see is_materialized
   */
  void materialize();

  /**
   * Returns whether or not the sarray has been materialized.
   *
   * \see materialize
   */
  bool is_materialized() const;

  /**
   * Returns an gl_sarray which contains the first n rows of this gl_sarray.
   *
   * \param n  The number of rows to fetch.
   * \code
   * gl_sarray sa({0,1,2,3,4,5,6,7,8,9});
   * auto ret = sa.head(5); // an array of values [0,1,2,3,4]
   * \endcode
   */
  gl_sarray head(size_t n) const;

  /**
   * Returns an gl_sarray which contains the last n rows of this gl_sarray.
   *
   * \param n  The number of rows to fetch.
   * \code
   * gl_sarray sa({0,1,2,3,4,5,6,7,8,9});
   * auto ret = sa.tail(5); // an array of values [5,6,7,8,9]
   * \endcode
   */
  gl_sarray tail(size_t n) const;
  /**
   *
   * Count words in the gl_sarray.
   *
   * \param to_lower Optional. If True, all words are converted to lower case
   * before counting.  
   *
   * Return an gl_sarray of dictionary type where each
   * element contains the word count for each word that appeared in the
   * corresponding input element. The words are split on all whitespace and
   * punctuation characters. Only works if this SArray is of string type.
   * Parameters:	
   *
   * \code
   * sa = turicreate.SArray(["The quick brown fox jumps.",
   *                     "Word word WORD, word!!!word"])
   * auto ret = count_words(sa)
   * // output array is of type flex_type_enum::DICT and contains
   * [{'quick': 1, 'brown': 1, 'jumps': 1, 'fox': 1, 'the': 1}, {'word': 5}]
   * \endcode
   *
   * \see count_ngrams
   */
  gl_sarray count_words(bool to_lower=true, turi::flex_list delimiters={"\r", "\v", "\n", "\f", "\t", " "}) const;


  /** 
   * Return an SArray of dict type where each element contains the count for
   * each of the n-grams that appear in the corresponding input element. The
   * n-grams can be specified to be either character n-grams or word n-grams. The
   * input SArray must contain strings.  Parameters:	
   * 
   * \param n Optional. The number of words in each n-gram. An n value of 1
   * returns word counts. Defaults to 2.
   * 
   * \param method Optional. Either "word" or "character". If “word”, the
   * function performs a count of word n-grams. If “character”, does a character
   * n-gram count. Defaults to "word".
   * 
   * \param to_lower Optional. If true, all words are converted to lower case
   * before counting. Defaults to true.
   * 
   * \param ignore_space Optional. If method is “character”, indicates if
   * spaces between words are counted as part of the n-gram. For instance, with
   * the input SArray element of “fun games”, if this parameter is set to False
   * one tri-gram would be ‘n g’. If ignore_space is set to True, there would be
   * no such tri-gram (there would still be ‘nga’). This parameter has no effect
   * if the method is set to “word”. Defaults to true.
   *
   * \code
   *  gl_sarray sa({"I like big dogs. I LIKE BIG DOGS."});
   *  gl_sarray ret = count_ngrams(sa, 3);
   *  // returns gl_sarray of dictionary type containing
   *  // [{'big dogs i': 1, 'like big dogs': 2, 'dogs i like': 1, 'i like big': 2}]
   * \endcode
   * \code
   *  gl_sarray sa(["Fun. Is. Fun"]);
   *  gl_sarray ret = count_ngrams(sa, 3, "character")
   *  // returns gl_sarray of dictionary type containing
   *  [{'fun': 2, 'nis': 1, 'sfu': 1, 'isf': 1, 'uni': 1}]
   * \endcode
   *
   mp* \see count_words
   */
  gl_sarray count_ngrams(size_t n=2, std::string method="word", 
                         bool to_lower=true, bool ignore_space=true) const;

  /**
   * Filter an SArray of dictionary type by the given keys. By default, all
   * keys that are in the provided list in "keys" are \b excluded from the
   * returned SArray.
   *
   *  \param keys A collection of keys to trim down the elements in the
   *  SArray.
   *
   *  \param exclude Optional If True, all keys that are in the input key list
   *  are removed. If False, only keys that are in the input key list are
   *  retained. Defaults to true.
   * 
   * \code
   *  gl_sarray sa({flex_dict{{"this",1}, {"is",1}, {"dog",2}},
   *                flex_dict{{"this", 2}, {"are",2}, {"cat", 1}} });
   *  gl_Sarray ret = sa.dict_trim_by_keys({"this", "is", "and", "are"});
   *  // returns an SArray of dictionary type  containing
   *  // [{'dog': 2}, {'cat': 1}]
   * \endcode
   *
   * \see dict_trim_by_values
   */
  gl_sarray dict_trim_by_keys(const std::vector<flexible_type>& keys,
                              bool exclude=true) const;

  /**
   * Filter dictionary values to a given range (inclusive). Trimming is only
   * performed on values which can be compared to the bound values. Fails on
   * SArrays whose data type is not ``dict``.
   * 
   * \param lower Optional. The lowest dictionary value that would be retained
   * in the result. If FLEX_UNDEFINED , lower bound is not applied. Defaults to
   * FLEX_UNDEFINED.
   *     
   * \param upper Optional. The highest dictionary value that would be retained
   * in the result.  If FLEX_UNDEFINED, upper bound is not applied. Defaults to
   * FLEX_UNDEFINED.
   * 
   * Example: 
   * \code
   * auto sa = gl_sarray({flex_dict{{"this",1},{"is",5},{"dog",7}},
   *                      flex_dict{{"this", 2},{"are",1},{"cat", 5}} });
   * std::cout << sa.dict_trim_by_values(2,5);
   * std::cout << sa.dict_trim_by_values(upper=5);
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: dict
   * Rows: 2
   * [{'is': 5}, {'this': 2, 'cat': 5}]
   * 
   * dtype: dict
   * Rows: 2
   * [{'this': 1, 'is': 5}, {'this': 2, 'are': 1, 'cat': 5}]
   * \endcode
   * 
   * \see dict_trim_by_keys
   */
  gl_sarray dict_trim_by_values(const flexible_type& lower = FLEX_UNDEFINED,
                                const flexible_type& upper = FLEX_UNDEFINED) const;
  /**
   * Create an gl_sarray that contains all the keys from each dictionary
   * element as a list. Fails on gl_sarray objects whose data type is not "dict".
   * 
   * 
   * Example: 
   * \code
   * auto sa = gl_sarray({flex_dict{{"this",1},{ "is",5},{ "dog",7}},
   *                      flex_dict{{"this", 2},{ "are", 1},{ "cat", 5}}});
   * std::cout << sa.dict_keys();
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: list
   * Rows: 2
   * [['this', 'is', 'dog'], ['this', 'are', 'cat']]
   * \endcode
   * 
   * \see dict_values
   */
  gl_sarray dict_keys() const;

  /**
   * Create an \ref gl_sarray that contains all the values from each dictionary
   * element as a list. Fails on \ref gl_sarray objects whose data type is not
   * "dict".
   * 
   * 
   * Example: 
   * \code
   * auto sa = gl_sarray({flex_dict{{"this",1},{"is",5},{"dog",7}}, 
   *                      flex_dict{{"this", 2},{"are", 1},{"cat", 5}}});
   * std::cout << sa.dict_values();
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: list
   * Rows: 2
   * [[1, 5, 7], [2, 1, 5]]
   * \endcode
   * 
   * \see dict_keys
   */
  gl_sarray dict_values() const;

  /**
   * Create a boolean \ref gl_sarray by checking the keys of an \ref gl_sarray
   * of dictionaries. An element of the output \ref gl_sarray is True if the
   * corresponding input element's dictionary has any of the given keys.  Fails
   * on \ref gl_sarray objects whose data type is not "dict".
   * 
   * \param keys A list of key values to check each dictionary against.
   * 
   * Example: 
   * \code
   * auto sa = gl_sarray({flex_dict{{"this",1},{ "is",5},{ "dog",7}}, 
   *                      flex_dict{{"animal",1}}, 
   *                      flex_dict{{"this", 2},{ "are", 1},{ "cat", 5}}});
   * std::cout << sa.dict_has_any_keys({"is", "this", "are"});
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: int
   * Rows: 3
   * [1, 1, 0]
   * \endcode
   * 
   * \see dict_has_all_keys
   */
  gl_sarray dict_has_any_keys(const std::vector<flexible_type>& keys) const;

  /**
   * Create a boolean \ref gl_sarray by checking the keys of an \ref gl_sarray
   * of dictionaries. An element of the output \ref gl_sarray is True if the
   * corresponding input element's dictionary has all of the given keys.  Fails
   * on \ref gl_sarray objects whose data type is not "dict".
   * 
   * \param keys A list of key values to check each dictionary against.
   * 
   * Example: 
   * \code
   * auto sa = gl_sarray({flex_dict{{"this",1},{"is",5},{"dog",7}}, 
   *                      flex_dict{{"this", 2},{"are", 1},{"cat", 5}}});
   * std::cout << sa.dict_has_all_keys({"is", "this"});
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: int
   * Rows: 2
   * [1, 0]
   * \endcode
   * 
   * \see dict_has_any_keys
   */
  gl_sarray dict_has_all_keys(const std::vector<flexible_type>& keys) const;


  /**
   * Transform each element of the \ref gl_sarray by a given function. The
   * result \ref gl_sarray is of type "dtype". "fn" should be a function that
   * returns exactly one value which can be cast into the type specified by
   * "dtype". 
   *  
   * \param fn The function to transform each element. Must return exactly one
   *     value which can be cast into the type specified by "dtype".
   *     
   * \param dtype The data type of the new \ref gl_sarray. 
   *      
   * \param skip_undefined Optional. If true, will not apply "fn" to 
   * any undefined values. Defaults to true.
   *     
   * Example: 
   * \code
   * auto sa = gl_sarray({1,2,3});
   * std::cout << sa.apply([](const flexible_type& x) { return x*1; }, 
   *                       flex_type_enum::INTEGER);
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: int
   * Rows: 3
   * [2, 4, 6]
   * \endcode
   * 
   * \see gl_sframe::apply
   */
  gl_sarray apply(std::function<flexible_type(const flexible_type&)> fn,
                  flex_type_enum dtype,
                  bool skip_undefined=true) const;

  /**
   * Filter this \ref gl_sarray by a function.  Returns a new \ref gl_sarray
   * filtered by this \ref gl_sarray.  If "fn" evaluates an element to true,
   * this element is copied to the new \ref gl_sarray. If not, it isn't. Throws
   * an exception if the return type of "fn" is not castable to a boolean
   * value.
   *
   * \param fn Function that filters the \ref gl_sarray. Must evaluate to bool
   * or int.
   *     
   * \param skip_undefined Optional. If true, will not apply fn to any
   * undefined values.
   *     
   * Example: 
   * \code
   * auto sa = gl_sarray({1,2,3});
   * std::cout <<  sa.filter([](flexible_type x){ return x < 3; });
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: int
   * Rows: 2
   * [1, 2]
   * \endcode
   *
   * This function is equivalent to the combination of a logical_filter and 
   * an apply.
   * \code
   * res = sa[sa.apply(fn)];
   * \endcode
   */
  gl_sarray filter(std::function<bool(const flexible_type&)> fn,
                  bool skip_undefined=true) const;

  /**
   * Create an \ref gl_sarray which contains a subsample of the current 
   * \ref gl_sarray.
   * 
   * \param fraction The fraction of the rows to fetch. Must be between 0 and 1.
   *     
   * Example: 
   * \code
   * auto  sa = gl_sarray::from_sequence(0, 10);
   * std::cout <<  sa.sample(.3);
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: int
   * Rows: 3
   * [2, 6, 9]
   * \endcode
   */
  gl_sarray sample(double fraction) const;

  /**
   * Create an \ref gl_sarray which contains a subsample of the current 
   * \ref gl_sarray.
   * 
   * \param fraction The fraction of the rows to fetch. Must be between 0 and 1.
   *     
   * \param seed The random seed for the random number generator. 
   * Deterministic output is obtained if this is set to a constant.
   * 
   * Example: 
   * \code
   * auto  sa = gl_sarray::from_sequence(0, 10);
   * std::cout <<  sa.sample(.3, 12345);
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: int
   * Rows: 3
   * [1,3,6,9]
   * \endcode
   */
  gl_sarray sample(double fraction, size_t seed, bool exact=false) const;

  /**
   * Return true if every element of the \ref gl_sarray evaluates to true. For
   * numeric \ref gl_sarray objects zeros and missing values ("None") evaluate
   * to false, while all non-zero, non-missing values evaluate to true. For
   * string, list, and dictionary \ref gl_sarray objects, empty values (zero
   * length strings, lists or dictionaries) or missing values ("None") evaluate
   * to false. All other values evaluate to true.  Returns true on an empty
   * \ref gl_sarray.
   * 
   * Example: 
   * \code
   * std::cout <<  gl_sarray({1, FLEX_UNDEFINED}).all();
   * std::cout <<  gl_sarray({1, 0}).all();
   * std::cout <<  gl_sarray({1, 2}).all();
   * std::cout <<  gl_sarray({"hello", "world"}).all();
   * std::cout <<  gl_sarray({"hello", ""}).all();
   * std::cout <<  gl_sarray({}).all();
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * 0 
   * 0
   * 1
   * 1
   * 0
   * 1
   * \endcode
   * 
   * \see any
   */
  bool all() const;

  /**
   * Return true if any element of the \ref gl_sarray evaluates to true. For
   * numeric \ref gl_sarray objects any non-zero value evaluates to true. For
   * string, list, and dictionary \ref gl_sarray objects, any element of
   * non-zero length evaluates to true.  Returns false on an empty \ref
   * gl_sarray.
   * 
   * Example: 
   * \code
   * std::cout <<  gl_sarray({1, FLEX_UNDEFINED}).any();
   * std::cout <<  gl_sarray({1, 0}).any();
   * std::cout <<  gl_sarray({0, 0}).any();
   * std::cout <<  gl_sarray({"hello", "world"}).any();
   * std::cout <<  gl_sarray({"hello", ""}).any();
   * std::cout <<  gl_sarray({"", ""}).any();
   * std::cout <<  gl_sarray({}).any();
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * 1
   * 1
   * 0
   * 1
   * 1
   * 0
   * 0
   * \endcode
   * 
   * \see all
   */
  bool any() const;

  /**
   * Get maximum numeric value in \ref gl_sarray.  Returns FLEX_UNDEFINED on an
   * empty \ref gl_sarray. Raises an exception if called on an \ref gl_sarray
   * with non-numeric type.
   * 
   * Example: 
   * \code
   * std::cout <<  gl_sarray({14, 62, 83, 72, 77, 96, 5, 25, 69, 66}).max();
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * 96
   * \endcode
   * 
   * \see min
   */
  flexible_type max() const;

  /**
   * Get minimum numeric value in \ref gl_sarray.  Returns FLEX_UNDEFINED on an
   * empty \ref gl_sarray. Raises an exception if called on an \ref gl_sarray
   * with non-numeric type.
   * 
   * Example: 
   * \code
   * std::cout <<  gl_sarray({14, 62, 83, 72, 77, 96, 5, 25, 69, 66}).min();
   * \endcode
   * 
   * 
   * \see max
   */
  flexible_type min() const;

  /**
   * Sum of all values in this \ref gl_sarray.
   *
   * Raises an exception if called on an \ref gl_sarray of strings, lists, or
   * dictionaries. If the \ref gl_sarray contains numeric arrays (flex_vec)
   * and all the arrays are the same length, the sum over all the arrays will
   * be returned. Returns FLEX_UNDEFINED on an empty \ref gl_sarray. For large
   * values, this may overflow without warning.
   */
  flexible_type sum() const;

  /**
   * Mean of all the values in the \ref gl_sarray, or mean image.  Returns
   * FLEX_UNDEFINED on an empty \ref gl_sarray. Raises an exception if called
   * on an \ref gl_sarray with non-numeric type or non-Image type.
   */
  flexible_type mean() const;

  /**
   * Standard deviation of all the values in the \ref gl_sarray.
   * Returns FLEX_UNDEFINED on an empty \ref gl_sarray. Raises an exception if
   * called on an \ref gl_sarray with non-numeric type.
   */
  flexible_type std() const;

  /**
   * Number of non-zero elements in the \ref gl_sarray.
   */
  size_t nnz() const;

  /**
   * Number of missing elements in the \ref gl_sarray.
   */
  size_t num_missing() const;


  /**
   * Create a new \ref gl_sarray with all the values cast to str. The string
   * format is specified by the 'str_format' parameter.
   * 
   * \param str_format The format to output the string. Default format is
   * "%Y-%m-%dT%H:%M:%S%ZP". See the strftime specification for details on 
   * the format string.
   * 
   * Example: 
   * \code
   *
   *  boost::posix_time::ptime t(boost::gregorian::date(2011, 1, 1));
   *  boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
   *  auto x = (t - epoch).total_seconds();
   *
   *  auto sa = gl_sarray({flex_date_time(x)});
   *  std::cout <<  sa.datetime_to_str("%e %b %Y");
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: str
   * Rows: 1
   * [" 1 Jan 2011"]
   * \endcode
   * 
   * \see str_to_datetime
   */
  gl_sarray datetime_to_str(const std::string& str_format="%Y-%m-%dT%H:%M:%S%ZP") const;

  /**
   * Create a new \ref gl_sarray with all the values cast to datetime. The
   * string format is specified by the 'str_format' parameter.
   * 
   * \param str_format The format to parse the string. Default format is
   * "%Y-%m-%dT%H:%M:%S%ZP". See the strptime specification for details on 
   * the format string.
   * 
   * Example: 
   *
   * \code
   * auto  sa = gl_sarray({"20-Oct-2011 09:30:10 GMT-05:30"});
   * std::cout <<  sa.str_to_datetime("%d-%b-%Y %H:%M:%S %ZP");
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: datetime
   * Rows: 1
   * [20111020T093010]
   * \endcode
   * 
   * \see datetime_to_str
   */
  gl_sarray str_to_datetime(const std::string& str_format="%Y-%m-%dT%H:%M:%S%ZP") const;


  /**
   * Create a new \ref gl_sarray with all the values cast to
   * \ref turi::image_type of uniform size.
   * 
   * \param width int The width of the new images.
   *     
   * \param height int The height of the new images.
   *     
   * \param channels int. Number of channels of the new images.
   *     
   * \param undefined_on_failure optional. defaults to true. If true,
   * return FLEX_UNDEFINED type instead of Image type on failure.  If false,
   * raises error upon failure.
   *     
   * \param allow_rounding optional. Default to false. If true, rounds
   * non-integer values when converting to Image type.  If false, raises error
   * upon rounding.
   */
  gl_sarray pixel_array_to_image(size_t width, size_t height, size_t channels=3,
                                 bool undefined_on_failure=true) const;

  /**
   * Create a new \ref gl_sarray with all values cast to the given type. Throws
   * an exception if the types are not castable to the given type.
   * 
   * \param dtype The type to cast the elements to in \ref gl_sarray
   *     
   * \param undefined_on_failure: Optional. Defaults to True. If set to true,
   * runtime cast failures will be emitted as missing values rather than
   * failing.
   * 
   * Example: 
   * \code
   * auto sa = gl_sarray({'1','2','3','4'});
   * std::cout <<  sa.astype(flex_type_enum::INTEGER);
   * \endcode
   *
   * Produces output: 
   * \code{.txt}
   * dtype: int
   * Rows: 4
   * [1, 2, 3, 4]
   * \endcode
   * 
   * Given an SArray of strings that look like dicts, convert to a dictionary
   * type:
   * \code
   * auto sa = gl_sarray({'flex_dict{{1:2 3,4}}', 'flex_dict{{a:b c,d}}'});
   * std::cout <<  sa.astype(flex_type_enum::DICT);
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: dict
   * Rows: 2
   * [{1: 2, 3: 4}, {'a': 'b', 'c': 'd'}]
   * \endcode
   * 
   */
  gl_sarray astype(flex_type_enum dtype, bool undefined_on_failure=true) const;

  /**
   * Create a new \ref gl_sarray with each value clipped to be within the given
   * bounds.  In this case, "clipped" means that values below the lower bound
   * will be set to the lower bound value. Values above the upper bound will be
   * set to the upper bound value. This function can operate on \ref gl_sarray
   * objects of numeric type as well as array type, in which case each
   * individual element in each array is clipped. By default "lower" and
   * "upper" are set to "float('nan')" which indicates the respective bound
   * should be ignored. The method fails if invoked on an \ref gl_sarray of
   * non-numeric type.
   * \param lower Optional. The lower bound used to clip. 
   *              Ignored if equal to FLEX_UNDEFINED (the default).
   *     
   * \param upper Optional. The upper bound used to clip. 
   *              Ignored if equal to FLEX_UNDEFINED (the default).
   * 
   * Example: 
   * \code
   * auto sa = gl_sarray({1,2,3});
   * std::cout <<  sa.clip(2,2);
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: int
   * Rows: 3
   * [2, 2, 2]
   * \endcode
   * 
   * \see clip_lower
   * \see clip_upper
   */
  gl_sarray clip(flexible_type lower=FLEX_UNDEFINED, 
                 flexible_type upper=FLEX_UNDEFINED) const;

  /**
   * Create new \ref gl_sarray with all values clipped to the given lower
   * bound. This function can operate on numeric arrays, as well as vector
   * arrays, in which case each individual element in each vector is clipped.
   * Throws an exception if the \ref gl_sarray is empty or the types are
   * non-numeric.
   * 
   * \param threshold The lower bound used to clip values.
   * 
   * Example: 
   * \code
   * auto sa = gl_sarray({1,2,3});
   * std::cout << sa.clip_lower(2);
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: int
   * Rows: 3
   * [2, 2, 3]
   * \endcode
   * 
   * \see clip
   * \see clip_upper
   */
  gl_sarray clip_lower(flexible_type threshold) const;

  /**
   * Create new \ref gl_sarray with all values clipped to the given upper
   * bound. This function can operate on numeric arrays, as well as vector
   * arrays, in which case each individual element in each vector is clipped.
   * 
   * \param threshold The upper bound used to clip values.
   * 
   * Example: 
   * \code
   * auto sa = gl_sarray({1,2,3});
   * std::cout << sa.clip_upper(2);
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: int
   * Rows: 3
   * [1, 2, 2]
   * \endcode
   * 
   * \see clip
   * \see clip_lower
   */
  gl_sarray clip_upper(flexible_type threshold) const;

  /**
   * Create new \ref gl_sarray containing only the non-missing values of the
   * \ref gl_sarray.  A missing value shows up in an \ref gl_sarray as
   * 'FLEX_UNDEFINED'.  This will also drop NAN values.
   */
  gl_sarray dropna() const;

  /**
   * Create new \ref gl_sarray with all missing values (FLEX_UNDEFINED or NaN)
   * filled in with the given value.  The size of the new \ref gl_sarray will
   * be the same as the original \ref gl_sarray. If the given value is not the
   * same type as the values in the \ref gl_sarray, "fillna" will attempt to
   * convert the value to the original \ref gl_sarray's type. If this fails, an
   * error will be raised.
   *
   * \param value The value used to replace all missing values
   */
  gl_sarray fillna(flexible_type value) const;

  /**
   * Create an \ref gl_sarray indicating which elements are in the top k.
   * Entries are '1' if the corresponding element in the current \ref gl_sarray is a
   * part of the top k elements, and '0' if that corresponding element is
   * not. Order is descending by default.
   *
   * \param topk Optional. Defaults to 10. The number of elements to determine
   * if 'top'
   *     
   * \param reverse Optional. Defaults to false. If true, return the topk
   * elements in ascending order
   */
  gl_sarray topk_index(size_t topk=10, bool reverse=false) const;
  
  /**
   * Append an \ref gl_sarray to the current \ref gl_sarray. Returns a new 
   * \ref gl_sarray with the rows from both \ref gl_sarray objects. Both 
   * \ref gl_sarray objects must be of the same type.
   * 
   * \param other Another \ref gl_sarray whose rows are appended to current \ref gl_sarray.
   * 
   * Example: 
   * \code
   * auto sa = gl_sarray({1, 2, 3});
   * auto sa2 = gl_sarray({4, 5, 6});
   * std::cout << sa.append(sa2);
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: int
   * Rows: 6
   * [1, 2, 3, 4, 5, 6]
   * \endcode
   * 
   * \see \ref gl_sframe.append
   */
  gl_sarray append(const gl_sarray& other) const;

  /**
   * Get all unique values in the current \ref gl_sarray.  Raises an error
   * if the \ref gl_sarray is of dictionary type. Will not necessarily preserve
   * the order of the given \ref gl_sarray in the new \ref gl_sarray.
   * 
   * \see gl_sframe::unique
   */
  gl_sarray unique() const;

  /**
   * Length of each element in the current \ref gl_sarray.  Only works on \ref
   * gl_sarray objects of dict, array, or list type. If a given element is a
   * missing value, then the output elements is also a missing value.  This
   * function is equivalent to the following:
   * 
   * sa_item_len =  sa.apply([](const flexible_type& x) {
   *                  return flexible_type(x.get_type() == flex_type_enum::UNDEFINED ? 0 : x.size();)
   *                });
   * 
   * Example: 
   * \code
   * auto sa = gl_sarray({flex_dict{{"is_restaurant", 1}, {"is_electronics", 0}},
   *                     flex_dict{{"is_restaurant", 1}, {"is_retail", 1}, {"is_electronics", 0}},
   *                     flex_dict{{"is_restaurant", 0}, {"is_retail", 1}, {"is_electronics", 0}},
   *                     flex_dict{{"is_restaurant", 0}},
   *                     flex_dict{{"is_restaurant", 1}, {"is_electronics", 1}},
   *                     FLEX_UNDEFINED});
   * std::cout << sa.item_length();
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: int
   * Rows: 6
   * [2, 3, 3, 1, 2, None]
   * \endcode
   * 
   */
  gl_sarray item_length() const;

  /**
   * Splits an \ref gl_sarray of datetime type to multiple columns, return a
   * new \ref gl_sframe that contains expanded columns. A \ref gl_sarray of datetime will be
   * split by default into an \ref gl_sframe of 6 columns, one for each
   * year/month/day/hour/minute/second element.
   * 
   * When splitting a \ref gl_sarray of datetime type, new columns are named:
   * prefix.year, prefix.month, etc. The prefix is set by the parameter
   * "column_name_prefix" and defaults to 'X'. If column_name_prefix is
   * FLEX_UNDEFINED or empty, then no prefix is used.
   * 
   * If tzone parameter is true, then timezone information is represented
   * as one additional column which is a float shows the offset from
   * GMT(0.0) or from UTC.
   *
   * \param column_name_prefix Optional. If provided, expanded column names
   * would start with the given prefix.  Defaults to "X".
   *     
   * \param limit: Optional. Limits the set of datetime elements to expand.
   *    Elements are 'year','month','day','hour','minute', and 'second'.
   *     
   * \param tzone: Optional. A boolean parameter that determines whether to
   * show timezone column or not.  Defaults to false.
   * 
   * Example: 
   * \code
   * auto sa = gl_sarray({"20-Oct-2011", "10-Jan-2012"});
   * auto date_sarray = sa.str_to_datetime("%d-%b-%Y");
   * auto split_sf = date_sarray.split_datetime("", {"day","year"});
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
   * 
   */
  gl_sframe split_datetime(const std::string& column_name_prefix = "X", 
                   const std::vector<std::string>& limit = {"year","month","day","hour","minute","second"},
                   bool tzone=false) const;

  /**
   * Convert an \ref gl_sarray of list, array, or dict type to an \ref
   * gl_sframe with multiple columns.
   *
   * "unpack" expands an \ref gl_sarray using the values of each
   * vector/list/dict as elements in a new \ref gl_sframe of multiple columns.
   * For example, an \ref gl_sarray of lists each of length 4 will be expanded
   * into an \ref gl_sframe of 4 columns, one for each list element. An \ref
   * gl_sarray of lists/arrays of varying size will be expand to a number of
   * columns equal to the longest list/array.  An \ref gl_sarray of
   * dictionaries will be expanded into as many columns as there are keys.
   * 
   * When unpacking an \ref gl_sarray of list or vector type, new columns are
   * named: "column_name_prefix".0, "column_name_prefix".1, etc. If unpacking a
   * column of dict type, unpacked columns are named "column_name_prefix".key1,
   * "column_name_prefix".key2, etc.
   * 
   * When unpacking an \ref gl_sarray of list or dictionary types, missing
   * values in the original element remain as missing values in the resultant
   * columns.  If the "na_value" parameter is specified, all values equal to
   * this given value are also replaced with missing values. In an \ref
   * gl_sarray of vector type, NaN is interpreted as a missing value.
   * 
   * \ref gl_sframe::pack_columns() is the reverse effect of unpack.
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
   * \code
   * auto sa = gl_sarray({flex_dict{{"word", "a"},{"count", 1}}, 
   *                      flex_dict{{"word", "cat"},{"count", 2}}, 
   *                      flex_dict{{"word", "is"},{"count", 3}}, 
   *                      flex_dict{{"word", "coming"},{"count", 4}}});
   * std::cout <<  sa.unpack("");
   * \endcode
   * Produces output: 
   * \code{.txt}
   * Columns:
   *     count   int
   *     word    str
   * Rows: 4
   * Data:
   * +-------+--------+
   * | count |  word  |
   * +-------+--------+
   * |   1   |   a    |
   * |   2   |  cat   |
   * |   3   |   is   |
   * |   4   | coming |
   * +-------+--------+
   * [4 rows x 2 columns]
   * \endcode
   *
   * Unpack only the key "word":
   * \code
   * std::cout <<  sa.unpack("X", {}, FLEX_UNDEFINED, {"word"});
   * \endcode
   * Produces output: 
   * \code{.txt}
   * Columns:
   *     X.word  str
   * Rows: 4
   * Data:
   * +--------+
   * | X.word |
   * +--------+
   * |   a    |
   * |  cat   |
   * |   is   |
   * | coming |
   * +--------+
   * [4 rows x 1 columns]
   * \endcode
   *
   * Convert all zeros to missing values:
   * \code
   * auto  sa2 = gl_sarray({flex_vec{1, 0, 1}, 
   *                        flex_vec{1, 1, 1}, 
   *                        flex_vec{0, 1}});
   * std::cout <<  sa2.unpack("X", {flex_type_enum::INTEGER, 
   *                                flex_type_enum::INTEGER, 
   *                                flex_type_enum::INTEGER}, 0);
   * \endcode
   * Produces output: 
   * \code{.txt}
   * Columns:
   *     X.0     int
   *     X.1     int
   *     X.2     int
   * Rows: 3
   * Data:
   * +------+------+------+
   * | X.0  | X.1  | X.2  |
   * +------+------+------+
   * |  1   | None |  1   |
   * |  1   |  1   |  1   |
   * | None |  1   | None |
   * +------+------+------+
   * [3 rows x 3 columns]
   * \endcode
   */
  gl_sframe unpack(const std::string& column_name_prefix = "X", 
                   const std::vector<flex_type_enum>& column_types = std::vector<flex_type_enum>(),
                   const flexible_type& na_value = FLEX_UNDEFINED, 
                   const std::vector<flexible_type>& limit = std::vector<flexible_type>()) const;

  /**
   * Sort all values in this \ref gl_sarray.  Sort only works for sarray of
   * type str, int and float, otherwise TypeError will be raised. Creates a
   * new, sorted \ref gl_sarray.
   *
   * \param ascending Optional. Defaults to True. If true, the sarray values
   *     are sorted in ascending order, otherwise, descending order.
   * 
   * Example: 
   * \code
   * auto sa = SArray({3,2,1});
   * std::cout <<  sa.sort();
   * \endcode
   * 
   * Produces output: 
   * \code{.txt}
   * dtype: int
   * Rows: 3
   * [1, 2, 3]
   * \endcode
   * 
   */
  gl_sarray sort(bool ascending=true) const;


  /**
   *
   *  This returns an SArray with each element sliced accordingly to the
   *  slice specified. 
   *
   *  \param start The start position of the slice
   *  \param stop The stop position of the slice
   *  \param step The step size of the slice (default = 1)
   *
   *  \return an SArray with each individual vector/string/list sliced
   *  according to the arguments.
   *
   *  This is conceptually equivalent to the python equivalent of:
   *  \code
   *     g.apply(lambda x: x[start:step:stop])
   *  \endcode
   *
   *  The SArray must be of type list, vector, or string.
   *
   *  For instance:
   *  \code
   *  g = SArray({"abcdef","qwerty"});
   *  std::cout << g.subslice(0, 2);
   *  \endcode
   *
   *  Produces output:
   *  \code{.txt}
   *  dtype: str
   *  Rows: 2
   *  ["ab", "qw"]
   *  \endcode
   * 
   *  Negative indeices:
   *  \code
   *  std::cout << g.subslice(3,-1);
   *  \endcode
   *  Produces output:
   *  \code{.txt}
   *  dtype: str
   *  Rows: 2
   *  ["de", "rt"]
   *  \endcode
   *
   *  Arrays:
   *  \code
   *  g = SArray({{1,2,3}, {4,5,6}});
   *  std::cout << g.subslice(0, 1);
   *  \endcode
   *
   *  Produces output:
   *  \code{.txt}
   *  dtype: str
   *  Rows: 2
   *  [[1], [4]]
   *  \endcode
   */
  gl_sarray subslice(flexible_type start = FLEX_UNDEFINED, 
                     flexible_type stop = FLEX_UNDEFINED, 
                     flexible_type step = FLEX_UNDEFINED);
  
/**
 *
 *  An abstraction to perform cumulative aggregates.
 *    y <- x.cumulative_aggregate(f, w_0)
 *
 *  The abstraction is as follows:
 *    y[i+1], w[i+1] = func(x[i], w[i])
 *  where w[i] is some arbitary state.
 *
 * \param[in] Built in aggregate to use (e.g, sum, min, max etc.)
 * \return SArray 
 *
 * \code
 *   sa = SArray([1, 2, 3, 4, 5])
 *   sa.cumulative_aggregate(std::make_shared<groupby_operators::sum>());
 * \endcode
 *
 * produces an SArray that looks like the following:
 *  dtype: int
 *  [1, 3, 6, 10, 15]
 * \endcode
 * \endcode
 *
 */
 gl_sarray cumulative_aggregate(
     std::shared_ptr<group_aggregate_value> aggregator) const; 
 gl_sarray builtin_cumulative_aggregate(const std::string& name) const;

  /**
   *
   *  This returns an SArray where each element is a cumulative aggregate of
   *  all its previous elements. Only works in an SArray of numeric type or
   *  numeric-array types. 
   *
   * \return an SArray 
   *
   * \code
   *   sa = SArray([1, 2, 3, 4, 5])
   *   sa.cumulative_sum()
   * \endcode
   *
   * produces an SArray that looks like the following:
   *  dtype: int
   *  [1, 3, 6, 10, 15]
   * \endcode
   *
   */
  gl_sarray cumulative_sum() const;
  gl_sarray cumulative_min() const;
  gl_sarray cumulative_max() const;
  gl_sarray cumulative_var() const;
  gl_sarray cumulative_std() const;
  gl_sarray cumulative_avg() const;

  /**
   * Apply an aggregate function over a moving window.
   * 
   * \param input The input SArray (expects to be materialized)
   * \param fn_name string representation of the aggregation function to use.
   * The mapping is the same string mapping used by the groupby aggregate
   * function.
   * \param window_start The start of the moving window relative to the current
   * value being calculated, inclusive. For example, 2 values behind the current
   * would be -2, and 0 indicates that the start of the window is the current
   * value.
   * \param window_end The end of the moving window relative to the current value
   * being calculated, inclusive. Must be greater than `window_start`. For
   * example, 0 would indicate that the current value is the end of the window,
   * and 2 would indicate that the window ends at 2 data values after the
   * current.
   * \param min_observations The minimum allowed number of non-NULL values in the
   * moving window for the emitted value to be non-NULL. size_t(-1) indicates
   * that all values must be non-NULL.
   *
   * Returns an SArray of the same length as the input, with a type that matches
   * the type output by the aggregation function.
   * 
   * Throws an exception if:
   *  - window_end < window_start
   *  - The window size is excessively large (currently hardcoded to UINT_MAX).
   *  - The given function name corresponds to a function that will not operate
   *  on the data type of the input SArray.
   *  - The aggregation function returns more than one non-NULL types.
   *  
   *  Example:
   *  \code
   *  gl_sarray a{0,1,2,3,4,5,6,7,8,9};
   *  // Moving window encompasses 3 values behind current and current value.
   *  auto result = a.rolling_apply(std::string("__builtin__avg__"), -3, 0);
   *  \endcode
   *
   *  Produces an SArray with these values: 
   *  \code
   *  {NULL,NULL,NULL,1.5,2.5,3.5,4.5,5.5,6.5,7.5}
   *  \endcode
   */
  gl_sarray builtin_rolling_apply(const std::string &fn_name,
                                  ssize_t start,
                                  ssize_t end,
                                  size_t min_observations=size_t(-1)) const;


  /**
   * Show a visualization of the SArray.
   */
  void show(const std::string& path_to_client, const std::string& title, const std::string& xlabel, const std::string& ylabel) const;

  /**
   * \internal
   * Gets the internal implementation object.
   */
  virtual std::shared_ptr<unity_sarray> get_proxy() const;

 private:
  void instantiate_new();

  void ensure_has_sarray_reader() const;

  std::shared_ptr<unity_sarray> m_sarray;

  mutable std::shared_ptr<sarray_reader<flexible_type> > m_sarray_reader;

}; // gl_sarray



gl_sarray operator+(const flexible_type& opnd, const gl_sarray& opnd2);
gl_sarray operator-(const flexible_type& opnd, const gl_sarray& opnd2);
gl_sarray operator*(const flexible_type& opnd, const gl_sarray& opnd2);
gl_sarray operator/(const flexible_type& opnd, const gl_sarray& opnd2);
gl_sarray operator<(const flexible_type& opnd, const gl_sarray& opnd2);
gl_sarray operator>(const flexible_type& opnd, const gl_sarray& opnd2);
gl_sarray operator<=(const flexible_type& opnd, const gl_sarray& opnd2);
gl_sarray operator>=(const flexible_type& opnd, const gl_sarray& opnd2);
gl_sarray operator==(const flexible_type& opnd, const gl_sarray& opnd2);


/**
 * Provides printing of the gl_sarray.
 */
std::ostream& operator<<(std::ostream& out, const gl_sarray& other);







/**
 * \ingroup group_glsdk
 * A range object providing one pass iterators over part or all of a gl_sarray.
 *See \ref gl_sarray::range_iterator for usage examples.
 *
 * \see gl_sarray::range_iterator
 */
class gl_sarray_range {
 public:
  /// content type
  typedef flexible_type type;

  gl_sarray_range(std::shared_ptr<sarray_reader<flexible_type> > m_sarray_reader,
                  size_t start, size_t end);
  gl_sarray_range(const gl_sarray_range&) = default;
  gl_sarray_range(gl_sarray_range&&) = default;
  gl_sarray_range& operator=(const gl_sarray_range&) = default;
  gl_sarray_range& operator=(gl_sarray_range&&) = default;

  /// Iterator type
  struct iterator: 
      public boost::iterator_facade<iterator, 
                const flexible_type, boost::single_pass_traversal_tag> {
   public:
    iterator() = default;
    iterator(const iterator&) = default;
    iterator(iterator&&) = default;
    iterator& operator=(const iterator&) = default;
    iterator& operator=(iterator&&) = default;

    iterator(gl_sarray_range& range, bool is_start);
   private:
    friend class boost::iterator_core_access;
    void increment();
    void advance(size_t n);
    inline bool equal(const iterator& other) const {
      return m_counter == other.m_counter;
    }
    const type& dereference() const;
    size_t m_counter = 0;
    gl_sarray_range* m_owner = NULL;
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
  flexible_type m_current_value;
  std::shared_ptr<sarray_reader_buffer<flexible_type> > m_sarray_reader_buffer;
};


/**
 * Utility function to infer the most general type of an in memory vector of
 * flexible_types.
 */
flex_type_enum infer_type_of_list(const std::vector<flexible_type>& vec);


class gl_sarray_writer_impl;

/**
 * \ingroup group_glsdk
 * Provides the ability to write gl_sarrays.
 * The gl_sarray is internally cut into a collection of segments. Each segment
 * can be written to independently, and the resultant SArray is the effective
 * concatenation of all the segments.
 *
 * \code
 * // Writes an integer SArray of 4 segments.
 * gl_sarray_writer writer(flex_type_enum:INTEGER, 4);
 *
 * // for each segment, write a bunch of 10 values.
 * // segment 0 has 10 0's, 
 * // segment 1 has 10 1's,
 * // etc
 * for (size_t seg = 0;seg < 4; ++seg) {
 *   for (size_t i = 0;i < 10; ++i) {
 *     writer.write(i, seg);
 *   }
 * }
 *
 * gl_sarray sa = writer.close();
 * // sa is now an SArray of 40 elements comprising of 
 * // the sequence 10 0's, 10 1's, 10 2's, 10 3's
 * \endcode
 *
 * Different segments can be written safely in parallel. It is not safe to 
 * write to the same segment simultanously. 
 */
class gl_sarray_writer {
 public:
  /**
   * Constructs a writer to write an gl_sarray of a particular type.
   *
   * \param type The content type of the SArray. Everything written to the
   * writer (via \ref write) must be of that type, is implicitly castable to
   * that type, or is a missing value denoted with a FLEX_UNDEFINED value.
   * 
   * \param num_segments Optional. The number of segments of the SArray.
   * Adjusting this parameter has little performance impact on the resultant
   * gl_sarray. Modifying this value is only helpful for providing writer
   * parallelism. Defaults to the number of cores on the machine.
   */
  gl_sarray_writer(flex_type_enum type, size_t num_segments = (size_t)(-1));

  /**
   * Writes a single value to a given segment.
   *
   * For instance, 
   * \code
   * gl_sarray_writer writer(flex_type_enum:FLOAT, 1);
   * writer.write(1.5, 0); // writes the value 1.5 to segment 0
   * writer.write(1, 0); // writes the value 1.0 to segment 0 (integer can be cast to float)
   * \endcode
   *
   * Strings are the most general type and everything can cast to it. hence:
   * \code
   * gl_sarray_writer writer(flex_type_enum:STRING, 1);
   * writer.write("hello", 0); // writes the value "hello" to segment 0
   * writer.write(1.5, 0); // writes the value "1.5" to segment 0
   * writer.write(1, 0); // writes the value "1" to segment 0 
   * \endcode
   *
   * Different segments can be written safely in parallel. It is not safe to 
   * write to the same segment simultanously. 
   *
   * \param f The value to write. This value should be of the requested type
   * (as set in the constructor), or is castable to the requested type, or is
   * FLEX_UNDEFINED.
   *
   * \param segmentid The segment to write to.
   */
  void write(const flexible_type& f, size_t segmentid);

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
   * Stops all writes and returns the resultant SArray.
   */
  gl_sarray close();

  /**
   * Returns the number of segments of the SArray; this is the same value
   * provided on construction of the writer.
   */
  size_t num_segments() const;

  ~gl_sarray_writer();

 private:
  std::unique_ptr<gl_sarray_writer_impl> m_writer_impl;

};

} // namespace turi
#endif
