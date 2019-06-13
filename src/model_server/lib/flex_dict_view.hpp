/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_FLEX_DICT_HPP
#define TURI_UNITY_FLEX_DICT_HPP

#include <vector>
#include <core/data/flexible_type/flexible_type.hpp>

namespace turi {

  /**
   * A thin wrapper around flex_dict to facilitate access of the underneath
   * sparse vector.
   *
   * It can be used the following way, suppose sa_iter is an iterator on top
   * of sarray:
   *   flex_dict_view value = (*sa_iter);
   *
   * Internally, sparse vector points to a flex_dict structure. It will not make
   * copy of the data to avoid memory allocation
  **/
  class flex_dict_view {
  public:
    /**
     * Default constructor.
     */
    flex_dict_view() = delete;

    /**
     * Constructs a sparse vector from a flexible type that is DICT type
     */
    flex_dict_view(const flex_dict& value);

    /**
     * Constructs a sparse vector from a flexible type. This only works when
     * the value is of type flex_type_enum::DICT, it will throw otherwise
     */
    flex_dict_view(const flexible_type& value);

    /**
     * Given a key retrieve the value, throws if the key doesn't exist
     */
    const flexible_type& operator[](const flexible_type& key) const;

    /**
     * Returns whether or not a given key exists in the sparse vector
     */
    bool has_key(const flexible_type& key) const;

    /**
     * Returns number of elements in the sparse vector
     */
    size_t size() const;

    /**
     * Returns all keys in a vector
     */
    const std::vector<flexible_type>& keys();

    /**
     * Returns all values in a vector
     */
    const std::vector<flexible_type>& values();

    /**
     * Returns an iterator that gives out items from beginning
     */
    flex_dict::const_iterator begin() const;

    /**
     * Returns an iterator that points to end position
     */
    flex_dict::const_iterator end() const;

  private:
    const flex_dict* m_flex_dict_ptr;

    // keys and values are lazily materialized when queried
    std::vector<flexible_type> m_keys;
    std::vector<flexible_type> m_values;
  };
}



#endif
