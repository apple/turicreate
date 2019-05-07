/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_FLEXIBLE_TYPE_HPP
#define TURI_FLEXIBLE_TYPE_HPP
#include <string>
#include <vector>
#include <tuple>
#include <iostream>
#include <functional>
#include <typeindex>
#include <parallel/atomic.hpp>
#include <util/int128_types.hpp>
#include <flexible_type/flexible_type_base_types.hpp>
namespace turi {
void flexible_type_fail(bool);
}

// Note: Uses the same assert behavior in both release and debug,
// to ensure we never crash (always throw instead).
// See https://github.com/apple/turicreate/issues/1835
#define FLEX_TYPE_ASSERT(param) flexible_type_fail(param);

#ifdef NDEBUG
//  ---- RELEASE MODE ---
#ifdef TURI_COMPILE_EXTRA_OPTIMIZATION
  // also enable all the inline and flatten attributes
  #define FLEX_ALWAYS_INLINE __attribute__((always_inline))
  #define FLEX_ALWAYS_INLINE_FLATTEN __attribute__((always_inline,flatten))
#else
  #define FLEX_ALWAYS_INLINE
  #define FLEX_ALWAYS_INLINE_FLATTEN
#endif

#else
//  ---- DEBUG MODE ---
// disable the always_inline and flatten attributes
#define FLEX_ALWAYS_INLINE
#define FLEX_ALWAYS_INLINE_FLATTEN
#endif

namespace turi {

/**
 * \ingroup group_gl_flexible_type
 *
 * The flexible_type is an automatic, self-managing union between the
 * following types.
 *  - \ref flex_int
 *  - \ref flex_float
 *  - \ref flex_string
 *  - \ref flex_nd_vec
 *  - \ref flex_list
 *  - \ref flex_dict
 *  - \ref flex_image 
 *  - \ref flex_undefined
 *
 * It is nearly every operator overloaded which
 * allows it to behave very similarly to python data types.
 *
 * The internal representation of the flexible_type is simply:
 *  - 8 bytes of (flex_int, flex_double, flex_string* and flex_vec*)
 *  - 1 byte tag corresponding to a member of the enumeration \ref flex_type_enum
 *
 * \code
 *    flexible_type f = 1;
 *    flexible_type f2 = 2;
 *    f += f2; // f is now 3
 *    f += 1; // f is now 4
 *    if (f == 4) std::cout << "!";
 *    f = float(f); // convert f to float
 *    f += 2.5; // f is now 6.5
 *
 *    // convert to string
 *    f = std::string(f); f is now "5"
 *    f += "hello";  // f is now "5hello"
 *
 *    // vector test
 *    f = {1.1, 2.2};  // f is now a vector if {1.1, 2.2}
 *    f.push_back(3.3); // f is now a vector if {1.1, 2.2, 3.3}
 *    f.clear();  // f is now an empty vector
 *
 *    for (flexible_type i = 0;i < 10; ++i) {
 *      f.push_back(i);
 *    }
 *    // f is now a vector from 1.0 to 10.0
 *
 *    f = std::string("hello"); // f is now a string again
 *    f.mutable_get<flex_string>() = "boo";  // this gets a reference to the underlying storage
 *    f.mutable_get<flex_int>() = 5; // this will implode at runtime
 * \endcode
 *
 * ### Type Information and Contents
 * The type information for the contents of the flexible type can be obtained in
 * three ways.
 *  - \ref flexible_type::get_type() Returns a member of the enumeration flex_type_enum
 *  - \ref flexible_type::which() Returns a number corresponding to size_t(get_type())
 *                               Provided for compatibility with boost::variant
 *  - \ref flexible_type::type() Returns a std::type_index object corresponding to
 *                               the typeid of the stored object.
 *                               Provided for compatibility with boost::variant
 *
 * Knowing the type of the contents, the \ref flexible_type::get() function
 * can be used to extract a reference to the content.
 *
 * \code
 *   flexible_type f(5);
 *   f.mutable_get<flex_int>() = 10;
 *   std::cout << f.get<flex_int>();
 * \endcode
 *
 * If the incorrect type is provided, the \ref flexible_type::get() function
 * will throw a runtime exception.
 *
 * A large number of casting operators are also provided and these work as expected.
 * \code
 *   (int)f    // casts doubles and integers to a integer. strings and vectors will fail.
 *   (float)f  // casts doubles and integers to a double . strings and vectors will fail.
 *   (string)f // returns a printable string representation of all types.
 * \endcode
 *
 * However, there are certain ambiguous cases whch do cause issues when
 * using the explicit cast so, the \ref flexible_type::to() function is generally
 * preferred:
 * \code
 *   f.to<int>()
 *   f.to<float>()
 *   f.to<string>()
 * \endcode
 *
 * ### Undefined
 * A special undefined type is provided which is useful for identifying missing
 * values (or the None value/type in Python). Such a value can be created using:
 * \code
 * flexible_type f = flex_undefined();
 * // or
 * flexible_type f = FLEX_UNDEFINED;
 * \endcode
 *
 * ### Operators
 * Practically every operator is overloaded, and the behaviors are relatively
 * intuitive. See the documentation for each operator to determine legality.
 * There are also special operator overloads to simplify access to the vector.
 * For instance, flexible_type::operator[], flexible_type::clear() and
 * flexible_type::push_back().
 * In all cases, a runtime exception is thrown if an invalid type combination
 * is encountered.
 *
 * The basic rule of the flexible_type is that the internal type is always
 * preserved. Only an assignment can change the internal type of the object.
 * Also, for consistency, operations between two flexible types always return
 * a type corresponding to the flexible_type on the left of the operation.
 * i.e.
 * \code
 *   flexible_type f_int(5);
 *   flexible_type f_float(2.1);
 *
 *   flexible_type f_res = f_int * f_float;
 *   // the type of f_res is an integer
 *
 *   f_res = f_float * f_int;
 *   // the type of f_res is a float
 * \endcode
 *
 * This is true also for operations where the 2nd argument is an arbitrary type.
 *
 * \code
 *   flexible_type f_float(2.1);
 *
 *   flexible_type f_res = f_float + 2;
 *   // the type of f_res is a float
 * \endcode
 *
 * The only exception to the rule is when the first argument is not a flexible type
 * and the 2nd argument is a flexible type. In which case, the return type is the
 * a flexible type corresponding to the type on the right.
 *
 * \code
 *   flexible_type f_float(2.1);
 *
 *   flexible_type f_res = 2 + f_float;
 *   // the type of f_res is a float
 * \endcode
 *
 * In summary:
 * Where flexible_type_A is used to denote a flexible_type where the internal
 * contents are of type A, and OP is an arbitrary operator,
 *  - flexible_type_A OP flexible_type_B ==> flexible_type_A
 *  - flexible_type_A OP T ==> flexible_type_A
 *  - T OP flexible_type_A ==> flexible_type_A
 *
 * This does mean that the binary operators are not commutative when performed
 * across different types. ex:
 *  - (integer * float) produces an integer
 *  - (float * integer) produces an float
 *
 *
 * ### Visitors
 * Of the most powerful feature of the flexible_type, is the visitor mechanism
 * which is designed in a very similar manner to that of the boost::variant.
 * See \ref flexible_type::apply_visitor(Visitor) or 
 * \ref flexible_type::apply_mutating_visitor(Visitor) for details.
 *
 *
 * ### Capability Checking
 * Sometimes it can be useful to be able to query (at runtime, or at compile
 * time) whether certain flexible_type operations will succeed or not. A number
 * of useful functions are provided.
 *
 * Runtime Capability Queries:
 *  - \ref flex_type_has_binary_op . Checks if binary operators between certain 
 *  types are supported.
 *  - \ref flex_type_is_convertible . Checks if certain type conversions can
 *  be performed.
 *
 * Compile-time Capability Queries:
 *  - \ref is_valid_flex_type . Checks at compile time if a type is one of the
 *  internal flexible_type types.
 *  - \ref enum_to_type . Gets the internal type from an enumeration value.
 *  - \ref type_to_enum . Gets the enumeration value from the internal type.
 *  - \ref has_direct_conversion_to_flexible_type . Can be casted to a flexible_type.
 *
 * ### Performance
 * Performance of the flexible_type is generally extremely good. With modern 
 * compilers (sometimes with inlining limit bumped up) can sometimes produce
 * code which is equivalent to, or runs as fast as the version using native
 * types especially if certain type constraints are set-up before hand.
 * For instance:
 *
 * \code
 * if (x.get_type() == flex_type_enum::INTEGER) {
 *   ... do a whole bunch of stuff on x ...
 * }
 * \endcode
 * 
 * Integers and Floating point values are stored in-place inside the 
 * flexible_type. All other types are somewhat more complex (strings, 
 * vectors etc) and are optimized by copy on write. i.e.
 * \code
 * flexible_type a = "hello world";
 * flexible_type b = a;
 * \endcode
 * Both b and a will reference the same "hello world" until either one of them
 * gets mutated. As such, all inplace operations 
 * (\ref get(), \ref apply_visitor(Visitor)) require const, but also have a
 * mutating version (\ref mutating_get(), \ref apply_mutating_visitor(Visitor)).
 */
class flexible_type {
 private:
  // used only for decltype deduction
  // the instantiation does not actually exist
  static flex_int prototype_flex_int;
 public:
  /**
   * Default Constructor.
   * By default constructs an integer of value 0.
   */
  flexible_type() noexcept;

  /**
   * Construct from initializer List. Makes a vector
   */
  template <typename T>
  flexible_type(std::initializer_list<T>&& list);

  /**
   * Destructor.
   */
  ~flexible_type();

  /*
   * Type constructor. Constructs a flexible type with a content type.
   */
  explicit flexible_type(flex_type_enum start_type) noexcept;

  /**
   * Copy constructor. Assigns this to a copy of other.
   */
  flexible_type(const flexible_type& other) noexcept;


  /**
   * Copy constructor. Assigns this to a copy of other.
   */
  flexible_type(flexible_type& other) noexcept;

  template <size_t N>
  flexible_type(const char v[N]):flexible_type() {
    (*this) = flex_string(v, N);
  }

  flexible_type(const char* c):flexible_type() {
    (*this) = flex_string(c);
  }
  /**
   * Copy constructor. Assigns this from arbitrary type.
   * See \ref flexible_type::operator=
   */
  template <typename T>
  flexible_type(const T& other, typename std::enable_if<has_direct_conversion_to_flexible_type<T>::value>::type* = nullptr);


  /**
   * Move constructor. Just assigns myself to the other, destroying the other.
   */
  flexible_type(flexible_type&& other) noexcept;

  /**
   * Move constructor. (Const overload. Required since the T&& universal
   * reference has a tendency to capture everything)
   */
  flexible_type(const flexible_type&& other) noexcept;


  /**
   * Move constructor. Assigns this from arbitrary type
   */
  template <typename T>
  flexible_type(T&& other, typename std::enable_if<has_direct_conversion_to_flexible_type<typename std::remove_reference<T>::type>::value>::type* = nullptr);


  /**
   * Assign from another while preserving the type of the current flexible_type.
   * - If left side is numeric and right side is numeric, the operation works
   *   as expected.
   * - If left side is a string, the right side is converted to a string
   *   representation
   * - if left side is a vector and right side is a vector, the vector is copied.
   * - All other cases will result in an assertion failure.
   */
  flexible_type& soft_assign(const flexible_type& other);


  /**
   * Assignment operator. Copies the other to myself.
   * See \ref flexible_type::operator=
   */
  flexible_type& operator=(const flexible_type& other) noexcept;

  /**
   * Assignment operator. Copies the other to myself.
   * See \ref flexible_type::operator=
   */
  flexible_type& operator=(flexible_type& other) noexcept;


  /**
   * Move assignment. Assigns myself from the other, destroying the other.
   */
  flexible_type& operator=(flexible_type&& other) noexcept;


  /**
   * Move assignment. (Const overload. Required since the T&& universal
   * reference has a tendency to capture everything).
   */
  flexible_type& operator=(const flexible_type&& other) noexcept;


  /**
   * Assignment from arbitrary type.
   * Figures out what can be casted from provided argument and stores that.
   * Specifically, it tests the following operations in this order.
   *  - If T is an integral type, create a flexible_type of an integer
   *  - If T is a floating point type, create a flexible_type of a float
   *  - If T can be converted to a string, create a flexible_type of a string
   *  - If T can be converted to a flex_vec. create a flexible_type of a flex_vec.
   */
  template <typename T>
  typename std::enable_if<
      has_direct_conversion_to_flexible_type<T>::value, 
      flexible_type&>::type operator=(const T& other);


  template <size_t N>
  flexible_type& operator=(const char v[N]) {
    (*this) = flex_string(v, N);
    return (*this);
  }

  flexible_type& operator=(const char* c) {
    (*this) = flex_string(c);
    return (*this);
  }
  /**
   * operator= for the undefined type
   * See \ref flexible_type::operator=
   */
  flexible_type& operator=(flex_undefined other);


  /**
   * Move assignment from arbitrary type.
   * Figures out what can be casted from provided argument and stores that.
   * Specifically, it tests the following operations in this order.
   *  - If T is an integral type, create a flexible_type of an integer
   *  - If T is a floating point type, create a flexible_type of a float
   *  - If T can be converted to a string, create a flexible_type of a string
   *  - If T can be converted to a flex_vec. create a flexible_type of a flex_vec.
   */
  template <typename T>
  typename std::enable_if<has_direct_conversion_to_flexible_type<
                 typename std::remove_reference<T>::type>::value, 
      flexible_type&>::type  operator=(T&& other);

  /**
   * Deletes the contents of this class, resetting to a different type.
   *
   * Note: Also ensures that that the reference count becomes 1.
   */
  void reset(flex_type_enum target_type);

  /**
   * Deletes the contents of this class, resetting to an integer
   */
  void reset();

  /**
   * Converts the current flexible type to a datetime type containing
   * posix_timestamp value and timezone offset.
   * The timezone offset is integral in \b fifteen-minute increments.
   * i.e. a offset of 8 means timezone +2
   */
  flexible_type& set_date_time_from_timestamp_and_offset(const std::pair<flex_int,int32_t> & datetime,
                                                         const int32_t microsecond=0);


  /**
   * Where the current flexible_type contains a datetime type, returns
   * the datetime as a pair of posix timestamp value and its timezone offset.
   *
   * The timezone offset is integral in \b fifteen-minute increments.
   * i.e. a offset of 8 means timezone +2
   */
  std::pair<flex_int,int32_t> get_date_time_as_timestamp_and_offset() const;

  /**
   * Gets the date_time microsecond value
   */
  int32_t get_date_time_microsecond() const;

  /**
   * Swaps contents with another flexible_type
   */
  void swap(flexible_type& b);


  /**
   * Gets a modifiable reference to the value stored inside the flexible_type.
   * T must be one of the flexible_type types.
   * All other types will result in a compile type assertion failure.
   * If the stored type does not match T, it will result in a run-time
   * assertion failure.
   */
  template <typename T>
  T& mutable_get();

  /**
   * Gets a const reference to the value stored inside the flexible_type.
   * T must be one of the flexible_type types.
   * All other types will result in a compile type assertion failure.
   * If the stored type does not match T, it will result in a run-time
   * assertion failure.
   */
  template <typename T>
  const T& get() const;


  /**
   * Gets a modifiable type unchecked modifiable reference to the value stored 
   * inside the flexible_type.
   *
   * This is generally unsafe to use unless you *really* know what 
   * you are doing. Use \ref get() instead.
   * 
   * Note that this is only defined for flex_int and flex_float type. It really
   * does not make sense to use this anywhere else.
   */
  template <typename T>
  T& reinterpret_mutable_get();

  /**
   * Gets a type unchecked modifiable reference to the value stored 
   * inside the flexible_type.
   *
   * This is generally unsafe to use unless you *really* know what 
   * you are doing. Use \ref get() instead.
   *
   * Note that this is only defined for flex_int and flex_float type. It really
   * does not make sense to use this anywhere else.
   */
  template <typename T>
  const T& reinterpret_get() const;

  /**
   * \{
   * Converts the flexible_type to a particular type.
   * Behaves like the implicit cast operators, but explicit.
   * In particular, this gets around the thorny issue that there is no way 
   * to cast to an flex_vec, flex_list or flex_dict even though the
   * implicit cast operators exist. This is due to std::vector<T> having two
   * constructors:
   * \code
   *  // regular copy constructor
   *  vector<T>::vector(const std::vector<T>&) ...
   *
   *  // constructs a vector of a particular size
   *  vector<T>::vector(size_t N, T val = T()) ...
   * \endcode
   * Resulting in two possible implicit cast routes when doing:
   * \code
   *  flexible_type f;
   *  ...
   *  flex_vec v = f; // fails
   *  flex_vec v = (flex_vec)f; // fails
   * \endcode
   *
   * This function provides an explicit cast allowing the following to be 
   * written:
   * \code
   *  flex_vec v = f.to<flex_vec>(); 
   * \endcode
   *
   * Of course, the alternative of is always available.
   * \code
   *  flex_vec v = f.operator flex_vec();
   * \endcode
   * And indeed, the implementation of to() simply redirects the calls.
   */
  template <typename T>
  typename std::enable_if<!std::is_integral<T>::value && !std::is_floating_point<T>::value, T>::type
  to() const;

  template<typename T>
  typename std::enable_if<std::is_integral<T>::value, T>::type
  to() const;

  template<typename T>
  typename std::enable_if<std::is_floating_point<T>::value, T>::type
  to() const;
  /// \}

  /**
   * Returns the type of the underlying storage
   */
  flex_type_enum get_type() const;


  /**
   * Returns the type index of the underlying storage.
   * For compatibility with variant.
   */
  std::type_index type() const;

  /**
   * Returns an integer ID identifying the type of the underlying storage.
   * Equivalent to (size_t)(get_type()). For compatibility with variant.
   */
  size_t which() const;

  /**
   * Returns a 64 bit hash of the underlying value, switched on type.
   */
  size_t hash() const;

  /**
   * Returns a 128 bit hash of the underlying value, switched on type.
   * This hash is appropriate for unique identification.
   */
  uint128_t hash128() const;

  /**
   * Returns true if the value is equal to this type's equivalent of zero.
   */
  bool is_zero() const;

  /**
   * Returns true if the value is considered a "missing value" or NA.
   */
  bool is_na() const;

/**************************************************************************/
/*                                                                        */
/*                           Visitor Functions                            */
/*                                                                        */
/**************************************************************************/

  /**
   * Executes an apply visitor on the flexible type.
   * The apply visitor object must have a function void operator()(T& t) for
   * every type T the flexible type can contain. The operator() function
   * can be itself templated for convenience. For instance, the following
   * visitor will add the value "1" to the flexible_type if the flexible_type
   * contains an integer, and will ignore otherwise.
   * \code
   * struct add_one_to_integer {
   *   // do nothing
   *   template <typename T>
   *   void operator()(T& t) { }
   *
   *   void operator()(flex_int& t) { t += 1; }
   * };
   * \endcode
   *
   * The visitor can return values but all versions of the operator() function
   * must return exactly the same type. For instance:
   *
   * \code
   * struct add_one_and_return {
   *   // do nothing
   *   template <typename T>
   *   int operator()(T& t) { return 0; }
   *
   *   int operator()(flex_int& t) { t += 1; return t; }
   * };
   * \endcode
   *
   * In which case, apply_mutating_visitor will return a value:
   * \code
   * int ret = flex.apply_mutating_visitor(add_one_and_return());
   * \endcode
   *
   * This function assumes that the function will mutate the value. If
   * the function does not mutate the value, the alternative 
   * \ref apply_visitor(Visitor) should be used.
   */
  template <typename Visitor>
  auto apply_mutating_visitor(Visitor visitor) -> decltype(visitor(prototype_flex_int));


  /**
   * Executes a non-mutating apply visitor on the flexible type.
   * The apply visitor object must have a function void operator()(const T& t) for
   * every type T the flexible type can contain. The operator() function
   * can be itself templated for convenience. For instance, the following
   * visitor will return one plus the value of the flexible_type if it
   * contains an integer, or returns 0 otherwise.
   *
   * \code
   * struct non_mutating_add_one {
   *   // do nothing
   *   template <typename T>
   *   int operator()(T& t) { return 0; }
   *
   *   int operator()(flex_int& t) { return t + 1; }
   * };
   * \endcode
   *
   * \code
   * int ret = flex.apply_visitor(non_mutating_add_one());
   * \endcode
   *
   * This function requires that the function does not mutate the value. If
   * the function does mutate the value, the alternative 
   * \ref apply_mutating_visitor(Visitor) should be used.
   */
  template <typename Visitor>
  auto apply_visitor(Visitor visitor) const -> decltype(visitor(prototype_flex_int));



  /**
   * Executes a binary visitor on the flexible type.
   * The binary apply visitor object must have a function void
   * operator()(T& t, const U& u) for every pair of types T and U the flexible
   * type can contain. The operator() function can be itself templated for
   * convenience. For instance, the following visitor will do a += if both
   * types are integer.
   * \code
   * struct add_one_to_integer {
   *   // do nothing
   *   template <typename T, typename U>
   *   void operator()(T& t, const U& u) const { }
   *
   *   void operator()(flex_int& t, const flex_int& u) const { t += u; }
   * };
   * \endcode
   *
   * Just like the unary apply_mutating_visitor. the visitor can return a value,
   * but all versions of operator() must return an identical type.
   *
   * This function assumes that the function will mutate the value. If
   * the function does not mutate the value, the alternative 
   * \ref apply_visitor(Visitor, const flexible_type&) should be used.
   */
  template <typename Visitor>
  auto apply_mutating_visitor(Visitor visitor, const flexible_type& other) -> decltype(visitor(prototype_flex_int, flex_int()));

  /**
   * Executes a binary visitor on the flexible type.
   * The binary apply visitor object must have a function void
   * operator()(const T& t, const U& u) for every pair of types T and U the
   * flexible type can contain. The operator() function can be itself templated
   * for convenience. For instance, the following visitor will return the sum
   * of both values if only if both types are integral.
   * \code
   * struct add_one_to_integer {
   *   // do nothing
   *   template <typename T, typename U>
   *   int operator()(T& t, const U& u) const { return 0; }
   *
   *   int operator()(flex_int& t, const flex_int& u) const { return t + u; }
   * };
   * \endcode
   *
   * Just like the unary apply_visitor. the visitor can return a value,
   * but all versions of operator() must return an identical type.
   *
   * This function assumes that the function will not mutate the value. If
   * the function does mutate the value, the alternative 
   * \ref apply_mutating_visitor(Visitor, const flexible_type&) should be used.
   */
  template <typename Visitor>
  auto apply_visitor(Visitor visitor, const flexible_type& other) const -> decltype(visitor(prototype_flex_int, flex_int()));
 /**************************************************************************/
 /*                                                                        */
 /*                   Convenience Operator Overloads                       */
 /*                                                                        */
 /**************************************************************************/
  /// Implicit cast to integral types
  template<class T,
      typename std::enable_if<std::is_integral<T>::value>::type* = (void*)NULL>
  inline FLEX_ALWAYS_INLINE_FLATTEN operator T() const {
    return to<T>();
  }


  /// Implicit cast to floating point types
  template<class T,
      typename std::enable_if<std::is_floating_point<T>::value>::type* = (void*)NULL>
  inline FLEX_ALWAYS_INLINE_FLATTEN operator T() const {
    return to<T>();
  }

  /**
   * Implicit cast to string
   */
  operator flex_string() const;

  /**
   * Implicit cast to vector<double>
   */
  operator flex_vec() const;

  /**
   * Implicit cast to ndarray<double>
   */
  operator flex_nd_vec() const;

  /**
   * Implicit cast to vector<flexible_type>
   */
  operator flex_list() const;

  /**
   * Implicit cast to vector<pair<flexible_type, flexible_type>>
   */
  operator flex_dict() const;

  
  /**
   * Implicit cast to flex_date_time
   */
  operator flex_date_time() const;

  /**
   * Implicit cast to flex_image
   */
  operator flex_image() const;

  /**
   * negation operator.
   * - negation operator.
   * - If the value is a numeric, the negation will return a negated value
   *   of the same type.
   * - If the value is a vector, the negation will return a vector with every
   *   element negated.
   * - All other conditions will result in an assertion failure.
   */
  flexible_type operator-() const ;


  /**
   * plus-equal operator.
   * - If both are numeric types (int / flex_float), the += will behave as expected.
   * - If both are strings, the += will append.
   * - If both are vectors, the += will perform a vector addition.
   * - If the left hand side is a vector, and right hand side is an int/float,
   *   the right hand side value will be added to each value in the vector.
   * - All other conditions will result in an assertion failure.
   */
  flexible_type& operator+=(const flexible_type& other) ;


  /**
   * subtract-equal operator.
   * - If both are numeric types (int / float), the -= will behave as expected.
   * - If both are vectors, the -= will perform a vector subtraction.
   * - If the left hand side is a vector, and right hand side is an int/float,
   *   the right hand side value will be subtracted from each value in the vector.
   * - All other conditions will result in an assertion failure.
   */
  flexible_type& operator-=(const flexible_type& other) ;



  /**
   * divide-equal operator.
   * - If both are numeric types (int / float), the /= will behave as expected.
   * - If the left hand side is a vector, and right hand side is an int/float,
   *   the right hand side value will be divided from each value in the vector.
   * - All other conditions will result in an assertion failure.
   */
  flexible_type& operator/=(const flexible_type& other) ;



  /**
   * mod-equal operator.
   * - If both are numeric types (int / float), the %= will behave as expected.
   * - If the left hand side is a vector, and right hand side is an int/float,
   *   the right hand side value will be divided from each value in the vector.
   * - All other conditions will result in an assertion failure.
   */
  flexible_type& operator%=(const flexible_type& other) ;

  /**
   * times-equal operator.
   * - If both are numeric types (int / float), the *= will behave as expected.
   * - If the left hand side is a vector, and right hand side is an int/float,
   *   the right hand side value will be multiplied to each value in the vector.
   * - All other conditions will result in an assertion failure.
   */
  flexible_type& operator*=(const flexible_type& other) ;


  /**
   * Plus operator.
   * \code
   * a = (*this);
   * a += other;
   * return a;
   * \endcode
   *
   * Note that unlike regular C++ typing rules, the resultant type is
   * always the type of the left hand side. i.e. int + float = int,
   * and float + int = float.
   */
  flexible_type operator+(const flexible_type& other) const;

  /**
   * Subtract operator.
   * Equivalent to
   * \code
   * a = (*this);
   * a += other;
   * return a;
   * \endcode
   *
   * Note that unlike regular C++ typing rules, the resultant type is
   * always the type of the left hand side. i.e. int + float = int,
   * and float + int = float.
   */
  flexible_type operator-(const flexible_type& other) const;


  /**
   * Multiplication operator.
   * Equivalent to
   * \code
   * a = (*this);
   * a *= other;
   * return a;
   * \endcode
   *
   * Note that unlike regular C++ typing rules, the resultant type is
   * always the type of the left hand side. i.e. int + float = int,
   * and float + int = float.
   */
  flexible_type operator*(const flexible_type& other) const;



  /**
   * Division operator.
   * Equivalent to
   * \code
   * a = (*this);
   * a /= other;
   * return a;
   * \endcode
   *
   * Note that unlike regular C++ typing rules, the resultant type is
   * always the type of the left hand side. i.e. int + float = int,
   * and float + int = float.
   */
  flexible_type operator/(const flexible_type& other) const;


  /**
   * Modulus operator.
   * Equivalent to
   * \code
   * a = (*this);
   * a %= otrer;
   * return a;
   * \endcode
   *
   * Note that unlike regular C++ typing rules, the resultant type is
   * always the type of the left hand side. i.e. int + float = int,
   * and float + int = float.
   */
  flexible_type operator%(const flexible_type& other) const;



  /**
   * Compares if two flexible types are equivalent in value. i.e.
   * permitting comparisons between integer and floating point types.
   * Same as \ref flexible_type::approx_equal
   */
  bool operator==(const flexible_type& other) const;


  /**
   * Compares if two flexible types are not equivalent in value. i.e.
   * permitting comparisons between integer and floating point types.
   * Same as \ref flexible_type::approx_equal
   */
  bool operator!=(const flexible_type& other) const;


  /**
   * Compares if two flexible types are equivalent in value. i.e.
   * permitting comparisons between integer and floating point types.
   */
  bool approx_equal(const flexible_type& other) const;

  /**
   * Like operator== but requires also types to be equivalent.
   */
  bool identical(const flexible_type& other) const;



  /**
   * Less than operator.
   *  - If both this and other are int / floats. this behaves as expected
   *  - All other cases will result in an assertion failure
   */
  bool operator<(const flexible_type& other) const;

  /**
   * greater than operator.
   *  - If both this and other are int / floats. this behaves as expected
   *  - All other cases will result in an assertion failure
   */
  bool operator>(const flexible_type& other) const;

  /**
   * less than or equal than operator.
   *  - If both this and other are int / floats. this behaves as expected
   *  - All other cases will result in an assertion failure
   */
  bool operator<=(const flexible_type& other) const;

  /**
   * greater than or equal than operator.
   *  - If both this and other are int / floats. this behaves as expected
   *  - All other cases will result in an assertion failure
   */
  bool operator>=(const flexible_type& other) const;

  /// Preincrement. Equivalent to (*this)+=1
  flexible_type& operator++();


  /// Preincrement. Equivalent to (*this)+=1, returning a copy of the previous value
  flexible_type operator++(int);



  /// Preincrement. Equivalent to (*this)-=1
  flexible_type& operator--();


  /// Preincrement. Equivalent to (*this)-=1, returning a copy of the previous value
  flexible_type operator--(int);


  // template versions of all the operators to compare against
  // other types
  //
  /**
   * Subtract operator with arbitrary type.
   * Equivalent to (*this) - (flexible_type)(other);
   */
  template <typename T>
  flexible_type operator-(const T& other) const;
  /**
   * Addition operator with arbitrary type.
   * Equivalent to (*this) + (flexible_type)(other);
   */
  template <typename T>
  flexible_type operator+(const T& other) const;
  /**
   * Division operator with arbitrary type.
   * Equivalent to (*this) / (flexible_type)(other);
   */
  template <typename T>
  flexible_type operator/(const T& other) const;
  /**
   * Modulos operator with arbitrary type.
   * Equivalent to (*this) % (flexible_type)(other);
   */
  template <typename T>
  flexible_type operator%(const T& other) const;
  /**
   * Multiplication operator with arbitrary type.
   * Equivalent to (*this) * (flexible_type)(other);
   */
  template <typename T>
  flexible_type operator*(const T& other) const;
  /**
   * Equality comparison operator with arbitrary type.
   * Equivalent to (*this) == (flexible_type)(other);
   */
  template <typename T>
  bool operator==(const T& other) const;
  /**
   * Equality comparison operator with arbitrary type.
   * Equivalent to (*this) != (flexible_type)(other);
   */
  template <typename T>
  bool operator!=(const T& other) const;
  /**
   * Less than comparison operator with arbitrary type.
   * Equivalent to (*this) < (flexible_type)(other);
   */
  template <typename T>
  bool operator<(const T& other) const;
  /**
   * Greater than comparison operator with arbitrary type.
   * Equivalent to (*this) > (flexible_type)(other);
   */
  template <typename T>
  bool operator>(const T& other) const;
  /**
   * Less than or equal comparison operator with arbitrary type.
   * Equivalent to (*this) <= (flexible_type)(other);
   */
  template <typename T>
  bool operator<=(const T& other) const;
  /**
   * Greater than or equal comparison operator with arbitrary type.
   * Equivalent to (*this) >= (flexible_type)(other);
   */
  template <typename T>
  bool operator>=(const T& other) const;


  /**
   * Array indexing operator.
   *  - When the contents of the flexible_type is a vector,
   *    Returns a reference to the 'index' entry in the vector.
   *  - When the contents is a float, returns a reference to the float
   *    only if the index is 0. (i.e. letting the scalar float act
   *    like a vector of size 1. All other indexes will fail with an
   *    assertion failure.
   *  - All other types will result in an assertion failure.
   */
  flex_float& operator[](size_t index);

  /**
   * Array indexing operator.
   *  - When the contents of the flexible_type is a vector,
   *    Returns a reference to the 'index' entry in the vector.
   *  - When the contents is a float, returns a reference to the float
   *    only if the index is 0. (i.e. letting the scalar float act
   *    like a vector of size 1. All other indexes will fail with an
   *    assertion failure.
   *  - All other types will result in an assertion failure.
   */
  const flex_float& operator[](size_t index) const;


  /**
   * List indexing operator.
   *  - When the contents of the flexible_type is a list.
   *    Returns a reference to the 'index' entry in the vector.
   *  - All other types will result in an assertion failure.
   */
  flexible_type&  array_at(size_t index) ;


  /**
   * List indexing operator.
   *  - When the contents of the flexible_type is a list.
   *    Returns a reference to the 'index' entry in the vector.
   *  - All other types will result in an assertion failure.
   */
  const flexible_type&  array_at(size_t index) const ;

  /**
   * dict indexing operator.
   *  - When the contents of the flexible_type is a dictionary,
   *    Returns a reference to the 'index' entry in the dict array.
   *  - All other types will result in an assertion failure.
   */
  flexible_type&  dict_at(const flexible_type& index)  ;


  /**
   * dict indexing operator.
   *  - When the contents of the flexible_type is a dictionary,
   *    Returns a reference to the 'index' entry in the dict array.
   *  - All other types will result in an assertion failure.
   */
  const flexible_type&  dict_at(const flexible_type& index) const  ;



  /**
   * List indexing operator.
   *  - When the contents of the flexible_type is a list or dict
   *    Returns a reference to the 'index' entry in the vector.
   *  - All other types will result in an assertion failure.
   */
  flexible_type&  operator()(size_t index)  ;


  /**
   * List indexing operator.
   *  - When the contents of the flexible_type is a list or dict,
   *    Returns a reference to the 'index' entry in the vector.
   *  - All other types will result in an assertion failure.
   */
  const flexible_type&  operator()(size_t index) const  ;

  /**
   * DIct indexing operator.
   *  - When the contents of the flexible_type is a dict,
   *    Returns a reference to the 'index' entry in the dict array.
   *  - All other types will result in an assertion failure.
   */
  flexible_type&  operator()(const flexible_type& index)   ;


  /**
   * Dict indexing operator.
   *  - When the contents of the flexible_type is a dict,
   *    Returns a reference to the 'index' entry in the dict array.
   *  - All other types will result in an assertion failure.
   */
  const flexible_type&  operator()(const flexible_type& index) const  ;


  /**
   * Array length function.
   *  - When the contents of the flexible_type is a vector or a recursive array
   *    Returns the length of the vector.
   *  - When the contents is a float, returns 1
   *    (i.e. letting the scalar float act like a vector of size 1.)
   *  - All other types will result in an assertion failure.
   */
  size_t size() const;

  /**
   * Array resize function
   *  - When the contents of the flexible_type is a vector or a recursive array
   *    resizes the array
   *  - All other types will result in an assertion failure.
   */
  void resize(size_t s);

  /**
   * Array clear function.
   *  - When the contents of the flexible_type is a vector, clears the vector.
   *  - When the contents of the flexible_type is a recursive object,
   *    clears the recursive array
   *  - All other types will result in an assertion failure.
   */
  void clear();

  /**
   * Dict element erase function.
   *  - When the contents of the flexible_type is a dict, erases
   *  the element indexed by the index value.
   *  - All other types will result in an assertion failure.
   */
  void erase(const flexible_type& index);

  /**
   * Array insertion function.
   *  - When the contents of the flexible_type is a vector, inserts
   *    the element to the end of the vector.
   *  - When the contents of the flexible_type is a recursive array, inserts
   *    a float to the end of the vector.
   *  - All other types will result in an assertion failure.
   */
  void push_back(flex_float i);


  /**
   * Array / List insertion function.
   *  - When the contents of the flexible_type is a vector, and the element
   *  is a floating point number, inserts the element to the end of the vector.
   *  - When the contents of the flexible_type is a list, inserts
   *    a value to the end of the vector.
   *  - All other types will result in an assertion failure.
   */
  void push_back(const flexible_type& i);


  /**
   * Serializer. Saves the flexible_type in an archive object.
   */
  void save(oarchive& oarc) const;

  /**
   * Deserializer. Loads the flexible_type from an archive object.
   * Note that the type of the flexible_type may change.
   */
  void load(iarchive& iarc);

 private:
  /*
   * union over the data types.
   * val holds the actual value.
   */
  union union_type {
    union_type(){};
    flex_int intval;
    flex_float dblval;
    std::pair<atomic<size_t>, flex_string>* strval;
    std::pair<atomic<size_t>, flex_vec>* vecval;
    std::pair<atomic<size_t>, flex_nd_vec>* ndvecval;
    std::pair<atomic<size_t>, flex_list>* recval;
    std::pair<atomic<size_t>, flex_dict>* dictval;
    std::pair<atomic<size_t>, flex_image>* imgval;
    flex_date_time dtval;

    struct {
      char padding[sizeof(flex_date_time)];
      flex_type_enum stored_type;
    };
  } val;

  void clear_memory_internal();

  inline FLEX_ALWAYS_INLINE_FLATTEN void ensure_unique() {
    switch(val.stored_type){
     case flex_type_enum::STRING:
       if (val.strval->first.value == 1) return;
       else {
         union_type prev;
         prev = val;
         val.strval = new std::pair<atomic<size_t>, flex_string>(*(val.strval));
         val.strval->first.value = 1;
         decref(prev, flex_type_enum::STRING);
       }
       break;
     case flex_type_enum::VECTOR:
       if (val.vecval->first.value == 1) return;
       else {
         union_type prev;
         prev = val;
         val.vecval = new std::pair<atomic<size_t>, flex_vec>(*(val.vecval));
         val.vecval->first.value = 1;
         decref(prev, flex_type_enum::VECTOR);
       }
       break;
     case flex_type_enum::ND_VECTOR:
       if (val.ndvecval->first.value == 1) return;
       else {
         union_type prev;
         prev = val;
         val.ndvecval = new std::pair<atomic<size_t>, flex_nd_vec>(*(val.ndvecval));
         val.ndvecval->first.value = 1;
         decref(prev, flex_type_enum::ND_VECTOR);
       }
       break;
     case flex_type_enum::LIST:
       if (val.recval->first.value == 1) return;
       else {
         union_type prev;
         prev = val;
         val.recval = new std::pair<atomic<size_t>, flex_list>(*(val.recval));
         val.recval->first.value = 1;
         decref(prev, flex_type_enum::LIST);
       }
       break;
     case flex_type_enum::DICT:
       if (val.dictval->first.value == 1) return;
       else {
         union_type prev;
         prev = val;
         val.dictval = new std::pair<atomic<size_t>, flex_dict>(*(val.dictval));
         val.dictval->first.value = 1;
         decref(prev, flex_type_enum::DICT);
       }
       break;
     case flex_type_enum::IMAGE:
       if (val.imgval->first.value == 1) return;
       else {
         union_type prev;
         prev = val;
         val.imgval = new std::pair<atomic<size_t>, flex_image>(*(val.imgval));
         val.imgval->first.value = 1;
         decref(prev, flex_type_enum::IMAGE);
       }
       break;
     default:
       break;
       // do nothing
    }
  }

  static inline FLEX_ALWAYS_INLINE_FLATTEN void decref(union_type& v, flex_type_enum type) noexcept {
    switch(type){
     case flex_type_enum::STRING:
       if (v.strval->first.dec() == 0) {
         delete v.strval;
         v.strval = NULL;
        }
       break;
     case flex_type_enum::VECTOR:
       if (v.vecval->first.dec() == 0) {
         delete v.vecval;
         v.vecval = NULL;
       }
       break;
     case flex_type_enum::ND_VECTOR:
       if (v.ndvecval->first.dec() == 0) {
         delete v.ndvecval;
         v.ndvecval = NULL;
       }
       break;
     case flex_type_enum::LIST:
       if (v.recval->first.dec() == 0) {
         delete v.recval;
         v.recval = NULL;
       }
       break;
     case flex_type_enum::DICT:
       if (v.dictval->first.dec() == 0) {
         delete v.dictval;
         v.dictval = NULL;
       }
       break;
     case flex_type_enum::IMAGE:
       if (v.imgval->first.dec() == 0) {
         delete v.imgval;
         v.imgval = NULL;
       }
       break;
     default:
       break;
       // do nothing
    }
  }

  static inline FLEX_ALWAYS_INLINE_FLATTEN void incref(union_type& v, flex_type_enum type) noexcept {
    switch(type){
     case flex_type_enum::STRING:
       v.strval->first.inc();
       break;
     case flex_type_enum::VECTOR:
       v.vecval->first.inc();
       break;
     case flex_type_enum::ND_VECTOR:
       v.ndvecval->first.inc();
       break;
     case flex_type_enum::LIST:
       v.recval->first.inc();
       break;
     case flex_type_enum::DICT:
       v.dictval->first.inc();
       break;
     case flex_type_enum::IMAGE:
       v.imgval->first.inc();
     default:
       break;
       // do nothing
    }
  }
};

/**
 * \ingroup group_gl_flexible_type
 *
 * A global static variable to be used when an undefined
 * value is needed. i.e.
 * \code
 * flexible_type f = FLEX_UNDEFINED;
 * \endcode
 */
static flexible_type FLEX_UNDEFINED = flex_undefined();


} // namespace turi



namespace std {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmismatched-tags"
#endif

template<>
struct hash<turi::flexible_type> {
  inline FLEX_ALWAYS_INLINE size_t operator()(const turi::flexible_type& s) const {
    return s.hash();
  }
};

#ifdef __clang__
#pragma clang diagnostic pop
#endif

}


namespace std {
template<>
inline void swap(turi::flexible_type& a, turi::flexible_type& b) noexcept {
  a.swap(b);
}
}

/**************************************************************************/
/*                                                                        */
/*                       Implementation Begins Here                       */
/*                                                                        */
/**************************************************************************/


#include <flexible_type/flexible_type_detail.hpp>


namespace turi {

inline FLEX_ALWAYS_INLINE std::pair<flex_int,int32_t> flexible_type::get_date_time_as_timestamp_and_offset() const {
  std::pair<flex_int,int32_t> ret;
  ret.first = val.dtval.posix_timestamp();
  ret.second = val.dtval.time_zone_offset();
  return ret;
}

inline FLEX_ALWAYS_INLINE int32_t flexible_type::get_date_time_microsecond() const {
  return val.dtval.microsecond();
}


inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type& flexible_type::set_date_time_from_timestamp_and_offset (const std::pair<flex_int,int32_t> & datetime, const int32_t microseconds) {
  reset(flex_type_enum::DATETIME);
  val.dtval.set_posix_timestamp(datetime.first);
  val.dtval.set_time_zone_offset(datetime.second);
  val.dtval.set_microsecond(microseconds);
  return *this;
}



/**************************************************************************/
/*                                                                        */
/*       We Begin with Specializations of the getters                     */
/*                                                                        */
/**************************************************************************/


// flex_date_time
template <>
inline FLEX_ALWAYS_INLINE flex_date_time& flexible_type::mutable_get<flex_date_time>() {
  FLEX_TYPE_ASSERT(get_type() == flex_type_enum::DATETIME);
  return val.dtval;
}

template <>
inline FLEX_ALWAYS_INLINE const flex_date_time& flexible_type::get<flex_date_time>() const {
  FLEX_TYPE_ASSERT(get_type() == flex_type_enum::DATETIME);
  return val.dtval;
}

// INTEGER
template <>
inline FLEX_ALWAYS_INLINE flex_int& flexible_type::mutable_get<flex_int>() {
  FLEX_TYPE_ASSERT(get_type() == flex_type_enum::INTEGER);
  return val.intval;
}

template <>
inline FLEX_ALWAYS_INLINE const flex_int& flexible_type::get<flex_int>() const {
  FLEX_TYPE_ASSERT(get_type() == flex_type_enum::INTEGER);
  return val.intval;
}

template <>
inline FLEX_ALWAYS_INLINE flex_int& flexible_type::reinterpret_mutable_get<flex_int>() {
  return val.intval;
}

template <>
inline FLEX_ALWAYS_INLINE const flex_int& flexible_type::reinterpret_get<flex_int>() const {
  return val.intval;
}

// flex_float
template <>
inline FLEX_ALWAYS_INLINE flex_float& flexible_type::mutable_get<flex_float>() {
  FLEX_TYPE_ASSERT(get_type() == flex_type_enum::FLOAT);
  return val.dblval;
}


template <>
inline FLEX_ALWAYS_INLINE const flex_float& flexible_type::get<flex_float>() const {
  FLEX_TYPE_ASSERT(get_type() == flex_type_enum::FLOAT);
  return val.dblval;
}

template <>
inline FLEX_ALWAYS_INLINE flex_float& flexible_type::reinterpret_mutable_get<flex_float>() {
  return val.dblval;
}

template <>
inline FLEX_ALWAYS_INLINE const flex_float& flexible_type::reinterpret_get<flex_float>() const {
  return val.dblval;
}


// STRING
template <>
inline FLEX_ALWAYS_INLINE flex_string& flexible_type::mutable_get<flex_string>() {
  FLEX_TYPE_ASSERT(get_type() == flex_type_enum::STRING);
  ensure_unique();
  return val.strval->second;
}

template <>
inline FLEX_ALWAYS_INLINE const flex_string& flexible_type::get<flex_string>() const {
  FLEX_TYPE_ASSERT(get_type() == flex_type_enum::STRING);
  return val.strval->second;
}


// VECTOR
template <>
inline FLEX_ALWAYS_INLINE flex_vec& flexible_type::mutable_get<flex_vec>() {
  FLEX_TYPE_ASSERT(get_type() == flex_type_enum::VECTOR);
  ensure_unique();
  return val.vecval->second;
}


template <>
inline FLEX_ALWAYS_INLINE const flex_vec& flexible_type::get<flex_vec>() const {
  FLEX_TYPE_ASSERT(get_type() == flex_type_enum::VECTOR);
  return val.vecval->second;
}


// ND_VECTOR
template <>
inline FLEX_ALWAYS_INLINE flex_nd_vec& flexible_type::mutable_get<flex_nd_vec>() {
  FLEX_TYPE_ASSERT(get_type() == flex_type_enum::ND_VECTOR);
  ensure_unique();
  return val.ndvecval->second;
}


template <>
inline FLEX_ALWAYS_INLINE const flex_nd_vec& flexible_type::get<flex_nd_vec>() const {
  FLEX_TYPE_ASSERT(get_type() == flex_type_enum::ND_VECTOR);
  return val.ndvecval->second;
}


// LIST
template <>
inline FLEX_ALWAYS_INLINE flex_list& flexible_type::mutable_get<flex_list>() {
  FLEX_TYPE_ASSERT(get_type() == flex_type_enum::LIST);
  ensure_unique();
  return val.recval->second;
}


template <>
inline FLEX_ALWAYS_INLINE const flex_list& flexible_type::get<flex_list>() const {
  FLEX_TYPE_ASSERT(get_type() == flex_type_enum::LIST);
  return val.recval->second;
}


// DICT
template <>
inline FLEX_ALWAYS_INLINE flex_dict& flexible_type::mutable_get<flex_dict>() {
  FLEX_TYPE_ASSERT(get_type() == flex_type_enum::DICT);
  ensure_unique();
  return val.dictval->second;
}


template <>
inline FLEX_ALWAYS_INLINE const flex_dict& flexible_type::get<flex_dict>() const {
  FLEX_TYPE_ASSERT(get_type() == flex_type_enum::DICT);
  return val.dictval->second;
}


// IMAGE
template <>
inline FLEX_ALWAYS_INLINE flex_image& flexible_type::mutable_get<flex_image>() {
  FLEX_TYPE_ASSERT(get_type() == flex_type_enum::IMAGE);
  ensure_unique();
  return val.imgval->second;
}

template <>
inline FLEX_ALWAYS_INLINE const flex_image& flexible_type::get<flex_image>() const {
  FLEX_TYPE_ASSERT(get_type() == flex_type_enum::IMAGE);
  return val.imgval->second;
}



inline FLEX_ALWAYS_INLINE void flexible_type::clear_memory_internal() {
  static_assert(sizeof(flex_date_time) == 12, "sizeof(flex_date_time)");
  static_assert(sizeof(flexible_type::union_type) == sizeof(flex_date_time) + 4,
                "sizeof(flexible_type::union_type)");
  static_assert(static_cast<int>(flex_type_enum::INTEGER) == 0,
                "value of flex_type_enum::INTEGER");

  val.intval = 0;
  val.dtval.m_microsecond = 0;
  val.stored_type = flex_type_enum::INTEGER;
}

//constructors
inline FLEX_ALWAYS_INLINE flexible_type::flexible_type() noexcept {
  clear_memory_internal();
}

template <typename T>
inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type::flexible_type(std::initializer_list<T>&& list) :flexible_type() {
  reset(flex_type_enum::VECTOR);
  // always unique after a reset
  val.vecval->second = flex_vec(list);
}

inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type::~flexible_type() {
  reset();
}

inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type::flexible_type(flex_type_enum start_type) noexcept:flexible_type() {
  reset(start_type);
}

inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type::flexible_type(const flexible_type& other) noexcept:flexible_type() {
  (*this) = other;
}


inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type::flexible_type(flexible_type& other) noexcept:flexible_type() {
  (*this) = other;
}


template <typename T>
inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type::flexible_type(const T& other, typename std::enable_if<has_direct_conversion_to_flexible_type<T>::value>::type*) :flexible_type() {
  (*this) = other;
}


inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type::flexible_type(flexible_type&& other)  noexcept: flexible_type() {
  val = other.val;
  val.stored_type = other.get_type();
  other.val.stored_type = flex_type_enum::INTEGER;
}

inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type::flexible_type(const flexible_type&& other)  noexcept: flexible_type() {
  val = other.val;
  val.stored_type = other.get_type();
  incref(val, val.stored_type);
}

template <typename T>
inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type::flexible_type(T&& other, typename std::enable_if<has_direct_conversion_to_flexible_type<typename std::remove_reference<T>::type>::value>::type*) :flexible_type() {
  this->operator=(std::forward<T>(other));
}


inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type& flexible_type::soft_assign(const flexible_type& other) {
  if (&other == this) return *this;
  apply_mutating_visitor(flexible_type_impl::soft_assignment_visitor(), other);
  return *this;
}


inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type& flexible_type::operator=(const flexible_type& other)noexcept {
  if (&other == this) return *this;
  decref(val, val.stored_type);
  val = other.val;
  val.stored_type = other.get_type();
  incref(val, val.stored_type);
  return *this;
}

inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type& flexible_type::operator=(flexible_type& other)noexcept {
  if (__builtin_expect(&other == this, 0)) return *this;
  decref(val, val.stored_type);
  val = other.val;
  val.stored_type = other.get_type();
  incref(val, val.stored_type);
  return *this;
}

inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type& flexible_type::operator=(const flexible_type&& other)  noexcept{
  if (__builtin_expect(&other == this, 0)) return *this;
  decref(val, val.stored_type);
  val = other.val;
  val.stored_type = other.get_type();
  incref(val, val.stored_type);
  return *this;
}


inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type& flexible_type::operator=(flexible_type&& other)  noexcept{
  if (__builtin_expect(&other == this, 0)) return *this;
  decref(val, val.stored_type);
  val = other.val;
  val.stored_type = other.get_type();
  other.val.stored_type = flex_type_enum::INTEGER;
  return *this;
}

template <typename T>
inline FLEX_ALWAYS_INLINE_FLATTEN 
typename std::enable_if<has_direct_conversion_to_flexible_type<T>::value, 
         flexible_type&>::type
flexible_type::operator=(const T& other) {
  constexpr flex_type_enum desired_type = has_direct_conversion_to_flexible_type<T>::desired_type;

  reset(desired_type);

  mutable_get<typename enum_to_type<desired_type>::type>() = other;
  return *this;
}


inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type& flexible_type::operator=(flex_undefined other) {
   reset(flex_type_enum::UNDEFINED);
  return *this;
}


template <typename T>
inline FLEX_ALWAYS_INLINE_FLATTEN 
typename std::enable_if<has_direct_conversion_to_flexible_type<
                        typename std::remove_reference<T>::type>::value, 
                        flexible_type&>::type
flexible_type::operator=(T&& other) {
  typedef typename std::remove_reference<T>::type BASE_T;
  constexpr flex_type_enum desired_type = has_direct_conversion_to_flexible_type<BASE_T>::desired_type;

  reset(desired_type);

  mutable_get<typename enum_to_type<desired_type>::type>() = std::forward<T>(other);
  return *this;
}

inline FLEX_ALWAYS_INLINE_FLATTEN void flexible_type::reset(flex_type_enum target_type) {
  // delete old value
  decref(val, val.stored_type);
  clear_memory_internal();

  // switch types
  val.stored_type = target_type;

  // construct the new type
  switch(get_type()) {
   case flex_type_enum::STRING:
     val.strval = new std::pair<atomic<size_t>, flex_string>;
     val.strval->first.value = 1;
     break;
   case flex_type_enum::VECTOR:
     val.vecval = new std::pair<atomic<size_t>, flex_vec>;
     val.vecval->first.value = 1;
     break;
   case flex_type_enum::ND_VECTOR:
     val.ndvecval = new std::pair<atomic<size_t>, flex_nd_vec>;
     val.ndvecval->first.value = 1;
     break;
   case flex_type_enum::LIST:
     val.recval = new std::pair<atomic<size_t>, flex_list>;
     val.recval->first.value = 1;
     break;
   case flex_type_enum::DICT:
     val.dictval = new std::pair<atomic<size_t>, flex_dict>;
     val.dictval->first.value = 1;
     break;
   case flex_type_enum::DATETIME:
     new (&val.dtval) flex_date_time(0,0); // placement new to create flex_date_time
     break;
   case flex_type_enum::IMAGE:
     val.imgval = new std::pair<atomic<size_t>, flex_image>;
     val.imgval->first.value = 1;
     break;
   default:
     break;
  }
}


inline FLEX_ALWAYS_INLINE_FLATTEN void flexible_type::reset() {
  decref(val, val.stored_type);
  clear_memory_internal();
  // switch types
  val.stored_type = flex_type_enum::INTEGER;
}


inline FLEX_ALWAYS_INLINE_FLATTEN void flexible_type::swap(flexible_type& b) {
  std::swap(val, b.val);
}

template <typename T>
inline FLEX_ALWAYS_INLINE T& flexible_type::mutable_get() {
  __attribute__((unused))
  typedef flexible_type_impl::invalid_type_instantiation_assert<false> unused;
  __builtin_unreachable();
}

template <typename T>
inline FLEX_ALWAYS_INLINE const T& flexible_type::get() const {
  __attribute__((unused))
  typedef flexible_type_impl::invalid_type_instantiation_assert<false> unused;
  __builtin_unreachable();
}


template <typename T>
inline FLEX_ALWAYS_INLINE T& flexible_type::reinterpret_mutable_get() {
  __attribute__((unused))
  typedef flexible_type_impl::invalid_type_instantiation_assert<false> unused;
  __builtin_unreachable();
}

template <typename T>
inline FLEX_ALWAYS_INLINE const T& flexible_type::reinterpret_get() const {
  __attribute__((unused))
  typedef flexible_type_impl::invalid_type_instantiation_assert<false> unused;
  __builtin_unreachable();
}


inline flex_type_enum FLEX_ALWAYS_INLINE flexible_type::get_type() const {
  return val.stored_type;
}


inline FLEX_ALWAYS_INLINE std::type_index flexible_type::type() const {
  return apply_visitor(flexible_type_impl::get_type_index());
}

inline FLEX_ALWAYS_INLINE size_t flexible_type::which() const {
  return (size_t)(get_type());
}

inline FLEX_ALWAYS_INLINE size_t flexible_type::hash() const {
  return apply_visitor(flexible_type_impl::city_hash_visitor());
}

inline FLEX_ALWAYS_INLINE uint128_t flexible_type::hash128() const {
  return apply_visitor(flexible_type_impl::city_hash128_visitor());
}


/**************************************************************************/
/*                                                                        */
/*                           Visitor Functions                            */
/*                                                                        */
/**************************************************************************/

template <typename Visitor>
inline FLEX_ALWAYS_INLINE_FLATTEN auto flexible_type::apply_mutating_visitor(Visitor visitor) -> decltype(visitor(prototype_flex_int)) {
  switch(get_type()) {
   case flex_type_enum::INTEGER:
     return visitor(mutable_get<flex_int>());
   case flex_type_enum::FLOAT:
     return visitor(mutable_get<flex_float>());
   case flex_type_enum::STRING:
     return visitor(mutable_get<flex_string>());
   case flex_type_enum::VECTOR:
     return visitor(mutable_get<flex_vec>());
   case flex_type_enum::ND_VECTOR:
     return visitor(mutable_get<flex_nd_vec>());
   case flex_type_enum::LIST:
     return visitor(mutable_get<flex_list>());
   case flex_type_enum::DICT:
     return visitor(mutable_get<flex_dict>());
   case flex_type_enum::DATETIME:
     return visitor(mutable_get<flex_date_time>());
   case flex_type_enum::IMAGE:
     return visitor(mutable_get<flex_image>());
   case flex_type_enum::UNDEFINED:
     flex_undefined undef;
     return visitor(undef);
   default:
     FLEX_TYPE_ASSERT(false);
  }
  __builtin_unreachable();
}


/// \overload
template <typename Visitor>
inline FLEX_ALWAYS_INLINE_FLATTEN auto flexible_type::apply_visitor(Visitor visitor) const -> decltype(visitor(prototype_flex_int)) {
  switch(get_type()) {
   case flex_type_enum::INTEGER:
     return visitor(get<flex_int>());
   case flex_type_enum::FLOAT:
     return visitor(get<flex_float>());
   case flex_type_enum::STRING:
     return visitor(get<flex_string>());
   case flex_type_enum::VECTOR:
     return visitor(get<flex_vec>());
   case flex_type_enum::ND_VECTOR:
     return visitor(get<flex_nd_vec>());
   case flex_type_enum::LIST:
     return visitor(get<flex_list>());
   case flex_type_enum::DICT:
     return visitor(get<flex_dict>());
   case flex_type_enum::DATETIME:
     return visitor(get<flex_date_time>());
   case flex_type_enum::IMAGE:
     return visitor(get<flex_image>());
   case flex_type_enum::UNDEFINED:
     flex_undefined undef;
     return visitor(undef);
   default:
     FLEX_TYPE_ASSERT(false);
  }
  __builtin_unreachable();
}

template <typename Visitor>
inline FLEX_ALWAYS_INLINE_FLATTEN auto flexible_type::apply_mutating_visitor(Visitor visitor, const flexible_type& other) -> decltype(visitor(prototype_flex_int, flex_int())) {
  using flexible_type_impl::const_visitor_wrapper;
  switch(other.get_type()) {
  
   case flex_type_enum::INTEGER:
     return apply_mutating_visitor(const_visitor_wrapper<Visitor,
                                         flex_int>{visitor, other.get<flex_int>()});
   case flex_type_enum::FLOAT:
     return apply_mutating_visitor(const_visitor_wrapper<Visitor,
                                         flex_float>{visitor, other.get<flex_float>()});
   case flex_type_enum::STRING:
     return apply_mutating_visitor(const_visitor_wrapper<Visitor,
                                         flex_string>{visitor, other.get<flex_string>()});
   case flex_type_enum::VECTOR:
     return apply_mutating_visitor(const_visitor_wrapper<Visitor,
                                         flex_vec>{visitor, other.get<flex_vec>()});
   case flex_type_enum::ND_VECTOR:
     return apply_mutating_visitor(const_visitor_wrapper<Visitor,
                                         flex_nd_vec>{visitor, other.get<flex_nd_vec>()});
   case flex_type_enum::LIST:
     return apply_mutating_visitor(const_visitor_wrapper<Visitor,
                                         flex_list>{visitor, other.get<flex_list>()});
   case flex_type_enum::DICT:
     return apply_mutating_visitor(const_visitor_wrapper<Visitor,
                                         flex_dict>{visitor, other.get<flex_dict>()});
   case flex_type_enum::DATETIME:
     return apply_mutating_visitor(const_visitor_wrapper<Visitor,
                                         flex_date_time>{visitor, other.get<flex_date_time>()});
   case flex_type_enum::IMAGE:
     return apply_mutating_visitor(const_visitor_wrapper<Visitor,
                                         flex_image>{visitor, other.get<flex_image>()});

   case flex_type_enum::UNDEFINED:
     flex_undefined undef;
     return apply_mutating_visitor(const_visitor_wrapper<Visitor,
                                         flex_undefined>{visitor, undef});
   default:
     FLEX_TYPE_ASSERT(false);
  }
  __builtin_unreachable();
}

/// overload
template <typename Visitor>
inline FLEX_ALWAYS_INLINE_FLATTEN auto flexible_type::apply_visitor(Visitor visitor, const flexible_type& other) const -> decltype(visitor(prototype_flex_int, flex_int())) {
  using flexible_type_impl::const_visitor_wrapper;
  switch(other.get_type()) {
   case flex_type_enum::INTEGER:
     return apply_visitor(const_visitor_wrapper<Visitor,
                                         flex_int>{visitor, other.get<flex_int>()});
   case flex_type_enum::FLOAT:
     return apply_visitor(const_visitor_wrapper<Visitor,
                                         flex_float>{visitor, other.get<flex_float>()});
   case flex_type_enum::STRING:
     return apply_visitor(const_visitor_wrapper<Visitor,
                                         flex_string>{visitor, other.get<flex_string>()});
   case flex_type_enum::VECTOR:
     return apply_visitor(const_visitor_wrapper<Visitor,
                                         flex_vec>{visitor, other.get<flex_vec>()});
   case flex_type_enum::ND_VECTOR:
     return apply_visitor(const_visitor_wrapper<Visitor,
                                         flex_nd_vec>{visitor, other.get<flex_nd_vec>()});
   case flex_type_enum::LIST:
     return apply_visitor(const_visitor_wrapper<Visitor,
                                         flex_list>{visitor, other.get<flex_list>()});
   case flex_type_enum::DICT:
     return apply_visitor(const_visitor_wrapper<Visitor,
                                         flex_dict>{visitor, other.get<flex_dict>()});
   case flex_type_enum::DATETIME:
     return apply_visitor(const_visitor_wrapper<Visitor,
                                         flex_date_time>{visitor, other.get<flex_date_time>()});
   case flex_type_enum::IMAGE:
     return apply_visitor(const_visitor_wrapper<Visitor,
                                         flex_image>{visitor, other.get<flex_image>()});
   case flex_type_enum::UNDEFINED:
     flex_undefined undef;
     return apply_visitor(const_visitor_wrapper<Visitor,
                                         flex_undefined>{visitor, undef});
   default:
     FLEX_TYPE_ASSERT(false);
  }
  __builtin_unreachable();
}
/**************************************************************************/
/*                                                                        */
/*                   Explicit type cast operator                          */
/*                                                                        */
/**************************************************************************/
template<typename T>
inline FLEX_ALWAYS_INLINE_FLATTEN 
typename std::enable_if<std::is_integral<T>::value, T>::type
flexible_type::to() const {
  return apply_visitor(flexible_type_impl::get_int_visitor());
}

template<typename T>
inline FLEX_ALWAYS_INLINE_FLATTEN 
typename std::enable_if<std::is_floating_point<T>::value, T>::type
flexible_type::to() const {
  return apply_visitor(flexible_type_impl::get_float_visitor());
}

template <>
inline FLEX_ALWAYS_INLINE_FLATTEN flex_string flexible_type::to<flex_string>() const {
  return apply_visitor(flexible_type_impl::get_string_visitor());
}

template <>
inline FLEX_ALWAYS_INLINE_FLATTEN flex_vec flexible_type::to<flex_vec>() const {
  return apply_visitor(flexible_type_impl::get_vec_visitor());
}

template <>
inline FLEX_ALWAYS_INLINE_FLATTEN flex_nd_vec flexible_type::to<flex_nd_vec>() const {
  return apply_visitor(flexible_type_impl::get_ndvec_visitor());
}

template <>
inline FLEX_ALWAYS_INLINE_FLATTEN flex_list flexible_type::to<flex_list>() const {
  return apply_visitor(flexible_type_impl::get_recursive_visitor());
}

template <>
inline FLEX_ALWAYS_INLINE_FLATTEN flex_dict flexible_type::to<flex_dict>() const {
  return apply_visitor(flexible_type_impl::get_dict_visitor());
}

template <>
inline FLEX_ALWAYS_INLINE_FLATTEN flex_date_time flexible_type::to<flex_date_time>() const {
  return apply_visitor(flexible_type_impl::get_datetime_visitor());
}

template <>
inline FLEX_ALWAYS_INLINE_FLATTEN flex_image flexible_type::to<flex_image>() const {
  return apply_visitor(flexible_type_impl::get_img_visitor());
}

/**************************************************************************/
/*                                                                        */
/*                   Convenience Operator Overloads                       */
/*                                                                        */
/**************************************************************************/

inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type::operator flex_string() const {
  return to<flex_string>();
}

inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type::operator flex_vec() const {
  return to<flex_vec>();
}

inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type::operator flex_nd_vec() const {
  return to<flex_nd_vec>();
}

inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type::operator flex_list() const {
  return to<flex_list>();
}

inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type::operator flex_dict() const {
  return to<flex_dict>();
}

inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type::operator flex_date_time() const {
  return to<flex_date_time>();
}

inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type::operator flex_image() const {
  return to<flex_image>();
}

inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type flexible_type::operator-() const {
  flexible_type copy(*this);
  copy.apply_mutating_visitor(flexible_type_impl::negation_operator());
  return copy;
}


inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type& flexible_type::operator+=(const flexible_type& other) {
  apply_mutating_visitor(flexible_type_impl::plus_equal_operator(), other);
  return *this;
}


inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type& flexible_type::operator-=(const flexible_type& other) {
  apply_mutating_visitor(flexible_type_impl::minus_equal_operator(), other);
  return *this;
}



inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type& flexible_type::operator/=(const flexible_type& other) {
  apply_mutating_visitor(flexible_type_impl::divide_equal_operator(), other);
  return *this;
}



inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type& flexible_type::operator%=(const flexible_type& other) {
  apply_mutating_visitor(flexible_type_impl::mod_equal_operator(), other);
  return *this;
}

inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type& flexible_type::operator*=(const flexible_type& other) {
  apply_mutating_visitor(flexible_type_impl::multiply_equal_operator(), other);
  return *this;
}


inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type flexible_type::operator+(const flexible_type& other) const {
  flexible_type ret(*this);
  ret += other;
  return ret;
}

inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type flexible_type::operator-(const flexible_type& other) const {
  flexible_type ret(*this);
  ret -= other;
  return ret;
}


inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type flexible_type::operator*(const flexible_type& other) const {
  flexible_type ret(*this);
  ret *= other;
  return ret;
}



inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type flexible_type::operator/(const flexible_type& other) const {
  flexible_type ret(*this);
  ret /= other;
  return ret;
}


inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type flexible_type::operator%(const flexible_type& other) const {
  flexible_type ret(*this);
  ret %= other;
  return ret;
}



inline FLEX_ALWAYS_INLINE_FLATTEN bool flexible_type::operator==(const flexible_type& other) const {
  return approx_equal(other);
}


inline FLEX_ALWAYS_INLINE_FLATTEN bool flexible_type::operator!=(const flexible_type& other) const {
  return !approx_equal(other);
}

inline FLEX_ALWAYS_INLINE_FLATTEN bool flexible_type::approx_equal(const flexible_type& other) const {
  return apply_visitor(flexible_type_impl::approx_equality_operator(), other);
}

inline FLEX_ALWAYS_INLINE_FLATTEN bool flexible_type::identical(const flexible_type& other) const {
  return apply_visitor(flexible_type_impl::equality_operator(), other);
}

inline FLEX_ALWAYS_INLINE_FLATTEN bool flexible_type::operator<(const flexible_type& other) const {
  return apply_visitor(flexible_type_impl::lt_operator(), other);
}

inline FLEX_ALWAYS_INLINE_FLATTEN bool flexible_type::operator>(const flexible_type& other) const {
  return apply_visitor(flexible_type_impl::gt_operator(), other);
}

inline FLEX_ALWAYS_INLINE_FLATTEN bool flexible_type::operator<=(const flexible_type& other) const {
  return (*this) < other || this->approx_equal(other);
}

inline FLEX_ALWAYS_INLINE_FLATTEN bool flexible_type::operator>=(const flexible_type& other) const {
  return (*this) > other || this->approx_equal(other);
}

/// Preincrement. Equivalent to (*this)+=1
inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type& flexible_type::operator++() {
  apply_mutating_visitor(flexible_type_impl::increment_operator());
  return *this;
}


/// Preincrement. Equivalent to (*this)+=1, returning a copy of the previous value
inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type flexible_type::operator++(int) {
  flexible_type ret(*this);
  apply_mutating_visitor(flexible_type_impl::increment_operator());
  return ret;
}



/// Preincrement. Equivalent to (*this)-=1
inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type& flexible_type::operator--() {
  apply_mutating_visitor(flexible_type_impl::decrement_operator());
  return *this;
}


/// Preincrement. Equivalent to (*this)-=1, returning a copy of the previous value
inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type flexible_type::operator--(int) {
  flexible_type ret(*this);
  apply_mutating_visitor(flexible_type_impl::decrement_operator());
  return ret;
}


// template versions of all the operators to compare against
// other types
//
template <typename T>
inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type flexible_type::operator-(const T& other) const { return (*this) - (flexible_type)(other); }
template <typename T>
inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type flexible_type::operator+(const T& other) const { return (*this) + (flexible_type)(other); }
template <typename T>
inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type flexible_type::operator/(const T& other) const { return (*this) / (flexible_type)(other); }
template <typename T>
inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type flexible_type::operator%(const T& other) const { return (*this) % (flexible_type)(other); }
template <typename T>
inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type flexible_type::operator*(const T& other) const { return (*this) * (flexible_type)(other); }
template <typename T>
inline FLEX_ALWAYS_INLINE_FLATTEN bool flexible_type::operator==(const T& other) const { return (*this) == (flexible_type)(other); }
template <typename T>
inline FLEX_ALWAYS_INLINE_FLATTEN bool flexible_type::operator!=(const T& other) const { return (*this) != (flexible_type)(other); }
template <typename T>
inline FLEX_ALWAYS_INLINE_FLATTEN bool flexible_type::operator<(const T& other) const { return (*this) < flexible_type(other); }
template <typename T>
inline FLEX_ALWAYS_INLINE_FLATTEN bool flexible_type::operator>(const T& other) const { return (*this) > flexible_type(other); }
template <typename T>
inline FLEX_ALWAYS_INLINE_FLATTEN bool flexible_type::operator<=(const T& other) const { return (*this) <= flexible_type(other); }
template <typename T>
inline FLEX_ALWAYS_INLINE_FLATTEN bool flexible_type::operator>=(const T& other) const { return (*this) >= flexible_type(other); }

inline FLEX_ALWAYS_INLINE_FLATTEN flex_float& flexible_type::operator[](size_t index) {
  ensure_unique();
  switch(get_type()) {
   case flex_type_enum::VECTOR:
     return val.vecval->second[index];
   case flex_type_enum::ND_VECTOR:
     return val.ndvecval->second[index];
   case flex_type_enum::FLOAT:
     if (index == 0) return val.dblval;
   default:
     FLEX_TYPE_ASSERT(false);
  }
  __builtin_unreachable();
}

inline FLEX_ALWAYS_INLINE_FLATTEN const flex_float& flexible_type::operator[](size_t index) const {
  switch(get_type()) {
   case flex_type_enum::VECTOR:
     return val.vecval->second[index];
   case flex_type_enum::ND_VECTOR:
     return val.ndvecval->second[index];
   case flex_type_enum::FLOAT:
     if (index == 0) return val.dblval;
   default:
     FLEX_TYPE_ASSERT(false);
  }
  __builtin_unreachable();
}


inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type&  flexible_type::array_at(size_t index)  {
  ensure_unique();
  return val.recval->second[index];
}


inline FLEX_ALWAYS_INLINE_FLATTEN const flexible_type&  flexible_type::array_at(size_t index) const  {
  return val.recval->second[index];
}


inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type& flexible_type::dict_at(const flexible_type& s)  {
  ensure_unique();
  flex_dict& value = val.dictval->second;
  for(auto& pair : value) {
    if (pair.first == s) {
      return pair.second;
    }
  }

  // not exist key
  log_and_throw("key does not exist in the dictionary");
  __builtin_unreachable();
}


inline FLEX_ALWAYS_INLINE_FLATTEN const flexible_type&  flexible_type::dict_at(const flexible_type& s) const  {
  const flex_dict& value = val.dictval->second;
  for(auto& pair : value) {
    if (pair.first == s) {
      return pair.second;
    }
  }
  FLEX_TYPE_ASSERT(false);
  __builtin_unreachable();
}


inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type&  flexible_type::operator()(size_t index)  {
  ensure_unique();
  switch(get_type()) {
   case flex_type_enum::LIST:
     return array_at(index);
   default:
     FLEX_TYPE_ASSERT(false);
  }
  __builtin_unreachable();
}


inline FLEX_ALWAYS_INLINE_FLATTEN const flexible_type&  flexible_type::operator()(size_t index) const  {
  switch(get_type()) {
   case flex_type_enum::LIST:
     return array_at(index);
   default:
     FLEX_TYPE_ASSERT(false);
  }
  __builtin_unreachable();
}

inline FLEX_ALWAYS_INLINE_FLATTEN flexible_type&  flexible_type::operator()(const flexible_type& index)  {
  ensure_unique();
  switch(get_type()) {
   case flex_type_enum::DICT:
     return dict_at(index);
   default:
     FLEX_TYPE_ASSERT(false);
  }
  __builtin_unreachable();
}


inline FLEX_ALWAYS_INLINE_FLATTEN const flexible_type&  flexible_type::operator()(const flexible_type& index) const  {
  switch(get_type()) {
   case flex_type_enum::DICT:
     return dict_at(index);
   default:
     FLEX_TYPE_ASSERT(false);
  }
  __builtin_unreachable();
}


inline FLEX_ALWAYS_INLINE_FLATTEN size_t flexible_type::size() const {
  switch(get_type()) {
   case flex_type_enum::VECTOR:
     return val.vecval->second.size();
   case flex_type_enum::ND_VECTOR:
     return val.ndvecval->second.num_elem();
   case flex_type_enum::LIST:
     return val.recval->second.size();
   case flex_type_enum::DICT:
     return val.dictval->second.size();
   default:
     return 1;
  }
}

inline FLEX_ALWAYS_INLINE_FLATTEN void flexible_type::resize(size_t s) {
  ensure_unique();
  switch(get_type()) {
   case flex_type_enum::VECTOR:
     val.vecval->second.resize(s);
     return;
   case flex_type_enum::LIST:
     val.recval->second.resize(s);
     return;
   default:
     FLEX_TYPE_ASSERT(false);
  }
}

inline FLEX_ALWAYS_INLINE_FLATTEN void flexible_type::clear() {
  switch(get_type()) {
   case flex_type_enum::VECTOR:
   case flex_type_enum::ND_VECTOR:
   case flex_type_enum::LIST:
   case flex_type_enum::DICT:
     reset(get_type());
     return;
   default:
     FLEX_TYPE_ASSERT(false);
  }
}

inline FLEX_ALWAYS_INLINE_FLATTEN void flexible_type::push_back(flex_float i) {
  ensure_unique();
  switch(get_type()) {
   case flex_type_enum::VECTOR:
     val.vecval->second.push_back(i);
     return;
   case flex_type_enum::LIST:
     val.recval->second.push_back(flexible_type(i));
     return;
   default:
     FLEX_TYPE_ASSERT(false);
  }
}


inline FLEX_ALWAYS_INLINE_FLATTEN void flexible_type::push_back(const flexible_type& i) {
  ensure_unique();
  switch(get_type()) {
   case flex_type_enum::VECTOR:
     val.vecval->second.push_back((flex_float)i);
     return;
   case flex_type_enum::LIST:
     val.recval->second.push_back(i);
     return;
   default:
     FLEX_TYPE_ASSERT(false);
  }
}



inline FLEX_ALWAYS_INLINE void flexible_type::save(oarchive& oarc) const {
  // in earlier versions of the serializer, a 4 byte tag value was saved 
  // together with the flexible_type. This has now been changed.
  // However, to continue correctly deserializing previously saved 
  // flexible_types we identify it by shifting the type values by 128
  unsigned char c = (128 + (unsigned char)(get_type()));
  oarc << c;
  apply_visitor(flexible_type_impl::serializer{oarc});
}

inline FLEX_ALWAYS_INLINE void flexible_type::load(iarchive& iarc) {
  unsigned char c;
  // in earlier versions of the serializer, a 4 byte tag value was saved 
  // together with the flexible_type. This has now been changed.
  // However, to continue correctly deserializing previously saved 
  // flexible_types we identify it by shifting the type values by 128
  //
  // 12 Jan 2018: I suspect we can disable the old deserialization
  // code path now. "Earlier version" I think is at least 4 years old now.
  iarc >> c;
  if (c < 128) {
    int32_t tag_value;
    // old version of flexble_type saves the tag
    iarc >> tag_value;
    reset(flex_type_enum(c));
  } else {
    // new version does not
    reset(flex_type_enum(c - 128));
  }
  apply_mutating_visitor(flexible_type_impl::deserializer{iarc});
}


/**************************************************************************/
/*                                                                        */
/*                      External Operator Overloads                       */
/*                                                                        */
/**************************************************************************/

// Doxygen documentation is not generated for these functions

#define EXT_ENABLE_IF(x)  \
typename std::enable_if<has_direct_conversion_to_flexible_type<T>::value && std::is_same<S, flexible_type>::value, x>::type


/*
 * Prints the contents of the flexible type.
 * Equivalent to os << std::string(f);
 */
template <typename T>
inline FLEX_ALWAYS_INLINE
typename std::enable_if<std::is_same<T, flexible_type>::value, std::ostream&>::type
    operator<<(std::ostream& os, const T& f) {
  os << (std::string)(f);
  return os;
}

/*
 * Reversed overload of the + operator,
 * requiring flexible_type to be on the right side, and a non-flexible type on the left.
 * The return type is that of the flexible_type on the right.
 * See \ref flexible_type::operator+
 */
template <typename T, typename S>
inline FLEX_ALWAYS_INLINE_FLATTEN EXT_ENABLE_IF(flexible_type) operator+(const T& other, const S& f) {
  return f + other;
}

/*
 * Reversed overload of the - operator,
 * requiring flexible_type to be on the right side, and a non-flexible type on the left.
 * The return type is that of the flexible_type on the right.
 * See \ref flexible_type::operator-
 */
template <typename T, typename S>
inline FLEX_ALWAYS_INLINE_FLATTEN EXT_ENABLE_IF(flexible_type) operator-(const T& other, const S& f) {
  return (-f) + flexible_type(other);
}

/*
 * Reversed overload of the * operator,
 * requiring flexible_type to be on the right side, and a non-flexible type on the left.
 * The return type is that of the flexible_type on the right.
 * See \ref flexible_type::operator*
 */
template <typename T, typename S>
inline FLEX_ALWAYS_INLINE_FLATTEN EXT_ENABLE_IF(flexible_type) operator*(const T& other, const S& f) {
  return f.operator*(other);
}

/*
 * Reversed overload of the / operator,
 * requiring flexible_type to be on the right side, and a non-flexible type on the left.
 * The return type is that of the flexible_type on the right.
 * See \ref flexible_type::operator/
 */
template <typename T, typename S>
inline FLEX_ALWAYS_INLINE_FLATTEN EXT_ENABLE_IF(flexible_type) operator/(const T& other, const S& f) {
  flexible_type ret(f.get_type());
  ret.soft_assign(flexible_type(other));
  ret /= f;
  return ret;
}


/*
 * Reversed overload of the / operator,
 * requiring flexible_type to be on the right side, and a non-flexible type on the left.
 * The return type is that of the flexible_type on the right.
 * See \ref flexible_type::operator/
 */
template <typename T, typename S>
inline FLEX_ALWAYS_INLINE_FLATTEN EXT_ENABLE_IF(flexible_type) operator%(const T& other, const S& f) {
  flexible_type ret(f.get_type());
  ret.soft_assign(flexible_type(other));
  ret %= f;
  return ret;
}

/*
 * Reversed overload of the == operator,
 * requiring flexible_type to be on the right side, and a non-flexible type on the left.
 * see \ref flexible_type::operator==
 */
template <typename T, typename S>
inline FLEX_ALWAYS_INLINE_FLATTEN EXT_ENABLE_IF(bool) operator==(const T& other, const S& f) {
   return f.operator==(flexible_type(other));
}

/*
 * Reversed overload of the < operator,
 * requiring flexible_type to be on the right side, and a non-flexible type on the left.
 * see \ref flexible_type::operator<
 */
template <typename T, typename S>
inline FLEX_ALWAYS_INLINE_FLATTEN EXT_ENABLE_IF(bool) operator<(const T& other, const S& f) { return (f > other); }

/*
 * Reversed overload of the > operator,
 * requiring flexible_type to be on the right side, and a non-flexible type on the left.
 * see \ref flexible_type::operator>
 */
template <typename T, typename S>
inline FLEX_ALWAYS_INLINE_FLATTEN EXT_ENABLE_IF(bool) operator>(const T& other, const S& f) { return (f < other); }

/*
 * Reversed overload of the <= operator,
 * requiring flexible_type to be on the right side, and a non-flexible type on the left.
 * see \ref flexible_type::operator<=
 */
template <typename T, typename S>
inline FLEX_ALWAYS_INLINE_FLATTEN EXT_ENABLE_IF(bool) operator<=(const T& other, const S& f) { return (f >= other); }
/*
 * \ingroup unity
 * Reversed overload of the >= operator,
 * requiring flexible_type to be on the right side, and a non-flexible type on the left.
 * see \ref flexible_type::operator>=
 */
template <typename T, typename S>
inline FLEX_ALWAYS_INLINE_FLATTEN EXT_ENABLE_IF(bool) operator>=(const T& other, const S& f) { return (f <= other); }
} // namespace turi

#undef EXT_ENABLE_IF
#undef FLEX_ALWAYS_INLINE
#undef FLEX_ALWAYS_INLINE_FLATTEN
#endif
