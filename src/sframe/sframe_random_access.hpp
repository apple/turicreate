/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_RANDOM_ACCESS_H_
#define TURI_SFRAME_RANDOM_ACCESS_H_

#include <flexible_type/flexible_type_base_types.hpp>
#include <flexible_type/flexible_type.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <util/basic_types.hpp>

using turi::flex_type_enum;
using turi::flexible_type;
using turi::gl_sarray;
using turi::gl_sframe;

#include <map>
#include <mutex>

using std::enable_shared_from_this;
using std::istream;
using std::make_shared;
using std::lock_guard;
using std::ostream;
using std::pair;
using std::recursive_mutex;
using std::shared_ptr;
using std::unique_lock;
using std::unordered_map;
using std::unordered_set;
using std::vector;
using std::weak_ptr;

namespace turi { namespace sframe_random_access {

/**
 * \ingroup sframe_physical
 * \addtogroup sframe_random_access SFrame Random Access Backend
 *
 * This module provides an alternate binary representation of (strongly-typed)
 * SArray and SFrame values, which supports fast random access and indirect
 * references, as well as some basic indexing utilities over this alternate
 * representation.
 * \{
 */

/*! \cond internal */
DECL_STRUCT(ref_context);
DECL_STRUCT(url);
DECL_STRUCT(value);
DECL_STRUCT(value_column);
DECL_STRUCT(value_either);
DECL_STRUCT(value_index);
DECL_STRUCT(value_nd_vector);
DECL_STRUCT(value_record);
DECL_STRUCT(value_ref);
DECL_STRUCT(value_type);

typedef typename boost::make_recursive_variant<
  value_column_p,
  value_nd_vector_p,
  value_record_p,
  value_either_p,
  value_ref_p,
  value_index_p
>::type value_v;

using value_id_map_shared_ptr_type = unordered_map<
  pair<int64_t, int64_t>,
  shared_ptr<value>,
  std_pair_hash<int64_t, int64_t>>;

using value_id_map_weak_ptr_type = unordered_map<
  pair<int64_t, int64_t>,
  weak_ptr<value>,
  std_pair_hash<int64_t, int64_t>>;
/*! \endcond */

/**
 * The \ref value struct is a tagged union of the following cases.
 */
enum class value_enum {
  /**
   * Column data (random-access variant of SArray).
   */
  COLUMN,
  /**
   * Multidimensional array data (also used to hold strings as 1-dimensional
   * character arrays).
   */
  ND_VECTOR,
  /**
   * Record mapping field names to values.
   * An SFrame is stored as a record mapping column names to column values.
   */
  RECORD,
  /**
   * Variant type storing one of multiple cases.
   */
  EITHER,
  /**
   * Indirect reference to another value or column subset.
   */
  REF,
  /**
   * Index structure providing fast lookups over a column (internal use only).
   */
  INDEX,
};

/**
 * Indexing modes for column data. Currently this is limited to equality-based
 * indexing (i.e., hash of each value in the column, but in the future we would
 * like to also support ordering-based indexing for numeric data.
 */
enum class index_mode_enum {
  /**
   * Index based on equality (i.e., hash of each value in the column).
   */
  EQUALS,
};

/**
 * Index lookup modes for column data. For equality-based indices this is
 * trivial, but for ordering-based indices we can look up based on, e.g.,
 * less-than or greater-than relations.
 */
enum class index_lookup_mode_enum {
  /**
   * Index based on equality (i.e., hash of each value in the column).
   */
  EQUALS,
};

/**
 * A simple convenience structure containing multiple hash maps, one for each
 * of our K worker threads. The space of valid hashes (0..2^128) is divided into
 * K equal sized chunks, and each thread writes to its own chunk, so that there
 * is no collision.
 */
template<typename T>
struct parallel_hash_map {
  /**
   * Insert a pair (k, v) into the hash map.
   */
  void put(uint128_t k, const T& v);

  /**
   * Get a reference to the value mapped at hash k.
   */
  T& operator[](uint128_t k);

  /**
   * Get an iterator corresponding to the value mapped at hash k.
   */
  typename unordered_map<uint128_t, T>::iterator find(uint128_t k);

  /**
   * Return the end iterator of the sub-map that corresponds to key k's chunk of
   * the hash space.
   */
  typename unordered_map<uint128_t, T>::iterator end(uint128_t k);

  /**
   * Return the number of occurrences of key k in the hash map.
   */
  int64_t count(uint128_t k);

  /**
   * Clear the hash map.
   */
  void clear();

  /**
   * Construct an empty hash map.
   */
  parallel_hash_map();

  /* \cond internal */
  vector<unordered_map<uint128_t, T>> maps_;
  /*! \endcond */
};

/**
 * Creates a random-access SFrame object from a standard \ref gl_sframe.
 */
value_p from_sframe(const gl_sframe& sf);

/**
 * Converts a random-access SFrame object to a standard \ref gl_sframe.
 */
gl_sframe to_sframe(value_p v);

/**
 * The \ref value struct stores an arbitary value in our random access backend
 * (including random-access variants of SFrame, SArray, and simple cases of
 * flexible_type). Specifically, a \ref value instance is a tagged union of the
 * cases described in \ref value_enum, together with several utility methods for
 * creating \ref value objects and performing relational operations on them.
 */
struct value : public enable_shared_from_this<value> {
  /**
   * Saves the contents of a random access SFrame value to disk.
   */
  void save(const string& output_path);

  /**
   * Loads the contents of a random access SFrame value from disk.
   */
  static value_p load_from_path(const string& input_path);

  /**
   * Creates a value corresponding to the empty record (e.g., a table with
   * zero columns).
   */
  static value_p create_empty_record();

  /**
   * Creates a scalar of type int64, with the value provided.
   */
  static value_p create_scalar_int64(int64_t x);

  /**
   * Creates a string with the value provided.
   */
  static value_p create_string(string s);

  /**
   * Creates a table value with the column names and values provided.
   */
  static value_p create_table(
    vector<string> column_names, vector<value_p> column_values);

  /**
   * Creates an integer column from the vector of int64 values provided.
   * The unique flag may be provided as an optimization to indicate that
   * the values in the vector are unique (no duplicates).
   */
  static value_p create_column_from_integers(vector<int64_t>& values, bool unique);

  /**
   * Returns the number of rows of a given column. It is invalid to call this
   * method on a value that is not a column.
   */
  int64_t get_column_length();

  /**
   * Returns the value corresponding to a given field of a record (e.g.,
   * retrieves the column from a table with the given column name).
   */
  value_p get_record_at_field_name(const string& field_name);

  /**
   * Returns the contents of this value as a raw string. It is invalid to call
   * this method on a value that is not a string.
   */
  string get_value_string();

  /**
   * Returns the contents of this value as a raw int64. It is invalid to call
   * this method on a value that is not an int64 scalar.
   */
  int64_t get_value_scalar_int64();

  /**
   * Returns the contents of this value as a raw float64. It is invalid to call
   * this method on a value that is not a float64 scalar.
   */
  double get_value_scalar_float64();

  /**
   * Returns the contents of this value as a raw integer. It is invalid to call
   * this method on a value that is not a scalar of some integer type.
   */
  uint64_t get_integral_value();

  /**
   * Build a hash-based index of the indicated source columns.
   */
  static value_p build_index(
    vector<value_p> source_columns, index_mode_enum index_mode);

  /**
   * Look up a given hash in an index built by \ref value::build_index.
   */
  static value_p index_lookup_by_hash(
    value_p index, uint128_t hash, index_lookup_mode_enum mode);

  /**
   * Look up a given set of values in an index built by \ref value::build_index.
   */
  static value_p index_lookup(
    value_p index, vector<value_p> keys, index_lookup_mode_enum mode);

  //////////////////// Implementation-internal APIs /////////////////////
  /*! \cond internal */
  static const char* object_id_;

  optional<string> struct_hash_cached_;

  value_v v_;
  value_type_p ty_;

  optional<url_p> url_context_;
  optional<int64_t> value_id_;
  optional<ref_context_p> ref_context_;

  value_enum which();
  template<typename T> T& as();
  template<typename T> const T& as() const;

  value_type_p get_type();

  static value_p create_record(value_type_p type, vector<value_p> fields);

  static value_p create_optional_none(value_type_p ty);
  static value_p create_optional_some(value_type_p ty, value_p v);

  static value_p create_index(
    value_p index_keys,
    value_p index_values_flat,
    value_p index_values_grouped,
    vector<uint128_t> index_hashes,
    parallel_hash_map<int64_t> index_map_singleton,
    parallel_hash_map<pair<int64_t, int64_t>> index_map_range,
    vector<value_type_p> source_column_types,
    index_mode_enum index_mode);

  optional<value_column*> get_as_direct_column();

  template<typename T> static value_p create(
    T v, value_type_p ty, optional<ref_context_p> accum_refs,
    optional<url_p> url_context, optional<int64_t> id);

  value(value_v v, value_type_p ty, optional<ref_context_p> accum_refs,
        optional<url_p> url_context, optional<int64_t> id);

  void save_raw(
    ostream& os, optional<ref_context_p> context,
    optional<unordered_set<int64_t>*> local_refs_acc);

  static value_p load_raw(
    istream& is, value_type_p type, optional<url_p> url_context);

  int64_t get_value_id();
  static value_p get_value_by_id(optional<url_p> url_context, int64_t value_id);

  // Note: should only access while holding lock
  static value_id_map_weak_ptr_type& get_value_id_map();
  static recursive_mutex& get_value_id_map_lock();
  /*! \endcond */
};

}}

#endif
