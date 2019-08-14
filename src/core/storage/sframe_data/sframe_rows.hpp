/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_sframe_rows_HPP
#define TURI_SFRAME_sframe_rows_HPP
#include <vector>
#include <map>
#include <core/data/flexible_type/flexible_type.hpp>
namespace turi {
class oarchive;
class iarchive;


/**
 * \ingroup sframe_physical
 * \addtogroup sframe_main Main SFrame Objects
 * \{
 */

/**
 *
 * sframe-rows is a semi-opaque wrapper around a collection of columns of
 * flexible_type (i.e. from an SFrame / SArray).  The objective is to allow the
 * underlying representation to be column-wise, while maintaining a row-wise
 * iterator interface.
 *
 * sframe_rows are fast and cheap to copy, and also allow values to be modified.
 * Internally, sframe_rows are built on a copy-on-write architecture thus
 * allowing for safe mutation. Most accessor methods have a "constant" version
 * which should be used if no value modifications are to be made. For instance:
 *  - sframe_rows::begin() vs sframe_rows::cbegin()
 *  - sframe_rows::get_columns() vs sframe_rows::cget_columns()
 *
 * The sframe_rows object is a relatively shallow wrapper over an
 *
 *    vector<shared_ptr<vector<flexible_type>>>
 *
 * where each shared_ptr<vector<flexible_type>> represents a single column.
 * The column set can be directly accessed and modified using
 * sframe_rows::get_columns() (returns a reference to the underlying vector)
 * or sframe_rows::cget_columns()
 *
 * \TODO: We *could* templatize this around the column type, allowing this to
 * be used for anything.
 */
class sframe_rows {
 public:
  /// The data type of decoded column (block_contents::DECODED_COLUMN)
  typedef std::vector<flexible_type> decoded_column_type;
  typedef std::shared_ptr<decoded_column_type> ptr_to_decoded_column_type;

  /**
   * Constructor
   */
  sframe_rows() = default;

  /**
   * Copy constructor. The copy constructor is fast as only pointers
   * are copied in a copy-on-write fashion.
   */
  sframe_rows(const sframe_rows& other) {
    m_decoded_columns = other.m_decoded_columns;
    m_is_unique = false;
    other.m_is_unique = false;
  }
  /**
   * Move constructor.
   */
  sframe_rows(sframe_rows&&) = default;

  /**
   * Assignment operator. The assignment operator is fast as only
   * pointers are copied in a copy on write fashion.
   */
  sframe_rows& operator=(const sframe_rows& other)  {
    m_decoded_columns = other.m_decoded_columns;
    m_is_unique = false;
    other.m_is_unique = false;
    return *this;
  }

  /**
   * Move assignment
   */
  sframe_rows& operator=(sframe_rows&&) = default;

  /// Returns the number of columns
  inline size_t num_columns() const {
    return m_decoded_columns.size();
  }

  /// Returns the number of rows
  inline size_t num_rows() const {
    if (m_decoded_columns.empty()) return 0;
    else if (m_decoded_columns[0] == nullptr) return 0;
    else return m_decoded_columns[0]->size();
  }

  /**
   * Clears the contents of the sframe_rows datastructure.
   */
  void clear();

  /**
   * Sets the size of sframe_rows. If num_rows == -1, columns are not resized.
   *
   * \note sframe_rows is a copy-on-write datastructure. This may trigger
   * a full copy of the contents of sframe_rows.
   */
  void resize(size_t num_cols, ssize_t num_rows = -1);

  /**
   * Adds to the right of the sframe_rows, a collection of decoded columns
   *
   * \code
   * add_decoded_column(std::move(source))
   * \endcode
   */
  void add_decoded_column(const ptr_to_decoded_column_type& decoded_column);


  /**
   * Returns a modifiable reference to the set of column groups
   *
   * \note sframe_rows is a copy-on-write datastructure. This may trigger
   * a full copy of the contents of sframe_rows.
   */
  inline std::vector<ptr_to_decoded_column_type>& get_columns() {
    if (!m_is_unique) ensure_unique();
    return m_decoded_columns;
  }

  /**
   * Returns a const reference to the set of column groups
   */
  inline const std::vector<ptr_to_decoded_column_type>& get_columns() const {
    return m_decoded_columns;
  }

  /**
   * Returns a const reference to the set of column groups
   */
  inline const std::vector<ptr_to_decoded_column_type>& cget_columns() const {
    return m_decoded_columns;
  }

  /**
   * Serializer
   */
  void save(oarchive& oarc) const;

  /**
   * Deserializer
   */
  void load(iarchive& oarc);

  struct iterator;
  struct const_iterator;

  /**
   * An row object which refererences a row of the sframe_rows
   * and mimics a std::vector<flexible_type>.
   *
   * \code
   * sframe_rows::row& r = rows[5];
   *
   * // assigns row 5 in the sframe_rows object "rows" to be the same as row 10
   * r = rows[10];
   *
   * // assigns row 5 in the sframe_rows object "rows" to be the same as row 7
   * // in some other sframe_rows object
   * r = some_other_rows[7]
   * \endcode
   */
  struct row {
   private:
    /// Creates a new row reference which references the same row as other.
    inline row(const row& other) = default;

    /// Creates a new row reference which references the same row as other.
    inline row(row&& other) = default;

   public:
    inline row() = default;


    /**
     * Makes the current row object have the same reference as another
     * row object.
     */
    void copy_reference(const row& other) {
      m_source = other.m_source;
      m_current_row_number = other.m_current_row_number;
    }

    /**
     * Assigns the value of this row. Modifies the row this row references
     * to have the same values as another row.
     */
    row& operator=(const row& other) {
      ASSERT_EQ(size(), other.size());
      for (size_t i = 0;i < size(); ++i) {
        (*this)[i] = other[i];
      }
      return *this;
    }

    /**
     * Moves another row value to this row. Modifies the row this row references
     * to have the same values as another row.
     */
    row& operator=(row&& other) {
      ASSERT_EQ(size(), other.size());
      for (size_t i = 0;i < size(); ++i) {
        (*this)[i] = std::move(other[i]);
      }
      return *this;
    }

    inline row(const sframe_rows* source, size_t current_row_number):
        m_source(source), m_current_row_number(current_row_number) { }

    /**
     * Implicit cast to std::vector<flexible_type>
     */
    inline operator std::vector<flexible_type>() const {
      std::vector<flexible_type> ret(size());
      for (size_t i = 0;i < ret.size(); ++i)  ret[i] = (*this)[i];
      return ret;
    }

    /**
     * Equivalent to operator[] but performs bounds checking
     */
    inline const flexible_type& at(size_t i) const {
      if (i < size()) return (*this)[i];
      else throw "Index out of bounds";
    }

    /**
     * Equivalent to operator[] but performs bounds checking
     */
    inline flexible_type& at(size_t i) {
      if (i < size()) return (*this)[i];
      else throw "Index out of bounds";
    }


    /**
     * Directly index column i of this row
     */
    inline const flexible_type& fast_at(size_t i) const {
      const auto& column = (*(m_source->m_decoded_columns[i]));
      return column[m_current_row_number];
    }

    /**
     * Directly index column i of this row
     */
    inline const flexible_type& operator[](size_t i) const{
      const auto& column = (*(m_source->m_decoded_columns[i]));
      return column[m_current_row_number];
    }

    /**
     * Directly index column i of this row
     */
    inline flexible_type& operator[](size_t i) {
      auto& column = (*(m_source->m_decoded_columns[i]));
      return column[m_current_row_number];
    }

    /**
     * Returns the number of columns in this row.
     */
    inline size_t size() const {
      return m_source->num_columns();
    }

    const sframe_rows* m_source = NULL;
    ssize_t m_current_row_number = 0;
    friend struct iterator;
    friend struct const_iterator;
    friend class sframe_rows;

    /**
     * Iterator over values of a row
     */
    struct const_iterator:
        public boost::iterator_facade<const_iterator,
                                      const flexible_type,
                                      boost::random_access_traversal_tag> {
          /// Pointer to the input range. NULL if end iterator.
          const row* m_source = nullptr;
          size_t m_current_idx = 0;
          const_iterator() { }
          const_iterator(const const_iterator&) = default;
          const_iterator(const_iterator&&) = default;
          const_iterator& operator=(const const_iterator&) = default;
          const_iterator& operator=(const_iterator&&) = default;
          /// default constructor
          explicit const_iterator(const sframe_rows::row& source, size_t current_idx = 0):
              m_source(&source), m_current_idx(current_idx) { };

         private:
          friend class boost::iterator_core_access;
          /// advances the iterator. See boost::iterator_facade
          inline void increment() {
            ++m_current_idx;
          }
          /// advances the iterator. See boost::iterator_facade
          inline void advance(size_t n) {
            m_current_idx += n;
          }

          /// Tests for iterator equality. See boost::iterator_facade
          inline bool equal(const const_iterator& other) const {
            return this->m_source == other.m_source &&
                this->m_current_idx == other.m_current_idx;
          }

          /// Dereference. See boost::iterator_facade
          inline const flexible_type& dereference() const {
            return m_source->fast_at(m_current_idx);
          }

          /// Dereference. See boost::iterator_facade
          const ssize_t distance_to(const const_iterator& other) const {
            return other.m_current_idx - m_current_idx;
          }
        };
    /**
     * Gets a constant iterator to the first element of the row.
     */
    inline const_iterator begin() const {
      return const_iterator(*this, 0);
    }

    /**
     * Gets a constant iterator to the last element of the row
     */
    inline const_iterator end() const {
      return const_iterator(*this, size());
    }


  };

  /**
   * A constant iterator across rows of sframe_rows
   */
  struct const_iterator:
      public boost::iterator_facade<const_iterator,
                                    const row,
                                    boost::random_access_traversal_tag> {
    /// Pointer to the input range. NULL if end iterator.
    const sframe_rows* m_source = NULL;
    row m_row;
    /// default constructor
    const_iterator() {}
    const_iterator(const const_iterator& other) {
      m_source = other.m_source;
      m_row.copy_reference(other.m_row);
    }
    const_iterator(const_iterator&& other) {
      m_source = other.m_source;
      m_row.copy_reference(other.m_row);
    }
    const_iterator& operator=(const const_iterator& other) {
      m_source = other.m_source;
      m_row.copy_reference(other.m_row);
      return *this;
    }
    const_iterator& operator=(const_iterator&& other) {
      m_source = other.m_source;
      m_row.copy_reference(other.m_row);
      return *this;
    }
    explicit const_iterator(const sframe_rows* source, size_t current_row_number = 0):
        m_row(source, current_row_number) { };
   private:
    friend class boost::iterator_core_access;
    /// advances the iterator. See boost::iterator_facade
    inline void increment() {
      ++m_row.m_current_row_number;
    }
    /// advances the iterator. See boost::iterator_facade
    inline void advance(size_t n) {
      m_row.m_current_row_number += n;
    }

    /// Tests for iterator equality. See boost::iterator_facade
    inline bool equal(const const_iterator& other) const {
        return this->m_source == other.m_source &&
            m_row.m_current_row_number == other.m_row.m_current_row_number;
    }

    /// Dereference. See boost::iterator_facade
    inline const row& dereference() const {
      return m_row;
    }

    /// Dereference. See boost::iterator_facade
    const ssize_t distance_to(const const_iterator& other) const {
      return other.m_row.m_current_row_number - m_row.m_current_row_number;
    }
  };

  /**
   * A non-constant interator over rows of sframe_rows
   */
  struct iterator:
      public boost::iterator_facade<iterator,
                                    row,
                                    boost::random_access_traversal_tag> {
    /// Pointer to the input range. NULL if end iterator.
    const sframe_rows* m_source = NULL;
    mutable row m_row;
    /// default constructor
    iterator() {}
    iterator(const iterator& other) {
      m_source = other.m_source;
      m_row.copy_reference(other.m_row);
    }
    iterator(iterator&& other) {
      m_source = std::move(other.m_source);
      m_row.copy_reference(other.m_row);
    }
    iterator& operator=(const iterator& other) {
      m_source = other.m_source;
      m_row.copy_reference(other.m_row);
      return *this;
    }
    iterator& operator=(iterator&& other) {
      m_source = std::move(other.m_source);
      m_row.copy_reference(other.m_row);
      return *this;
    }
    explicit iterator(sframe_rows* source, size_t current_row_number = 0):
        m_row(source, current_row_number) { };
   private:
    friend class boost::iterator_core_access;
    /// advances the iterator. See boost::iterator_facade
    inline void increment() {
      ++m_row.m_current_row_number;
    }
    /// advances the iterator. See boost::iterator_facade
    inline void advance(size_t n) {
      m_row.m_current_row_number += n;
    }

    /// Tests for iterator equality. See boost::iterator_facade
    inline bool equal(const iterator& other) const {
        return this->m_source == other.m_source &&
            m_row.m_current_row_number == other.m_row.m_current_row_number;
    }

    /// Dereference. See boost::iterator_facade
    inline row& dereference() const {
      return m_row;
    }

    /// Dereference. See boost::iterator_facade
    const ssize_t distance_to(const iterator& other) const {
      return other.m_row.m_current_row_number - m_row.m_current_row_number;
    }
  };

  /**
   * Gets a constant iterator to the first row of the sframe_rows.
   */
  inline const_iterator begin() const {
    return const_iterator(this, 0);
  }

  /**
   * Gets a constant iterator to the end of the sframe_rows.
   */
  inline const_iterator end() const {
    return const_iterator(this, num_rows());
  }

  /**
   * Gets a constant iterator to the first row of the sframe_rows.
   */
  inline const_iterator cbegin() const {
    return const_iterator(this, 0);
  }

  /**
   * Gets a constant iterator to the end of the sframe_rows.
   */
  inline const_iterator cend() const {
    return const_iterator(this, num_rows());
  }

  /**
   * Gets a mutable iterator to the first row of the sframe_rows.
   *
   * \note sframe_rows is a copy-on-write datastructure. This may trigger
   * a full copy of the contents of sframe_rows.
   */
  inline iterator begin() {
    if (!m_is_unique) ensure_unique();
    return iterator(this, 0);
  }

  /**
   * Gets a mutable iterator to the first row of the sframe_rows.
   *
   * \note sframe_rows is a copy-on-write datastructure. This may trigger
   * a full copy of the contents of sframe_rows.
   */
  inline iterator end() {
    if (!m_is_unique) ensure_unique();
    return iterator(this, num_rows());
  }

  /**
   * Reads a particular row of the sframe_rows object.
   */
  inline const row operator[](size_t i) const {
    return row(this, i);
  }

  /**
   * gets a mutable reference to a particular row of the sframe_rows object
   */
  inline row operator[](size_t i) {
    if (!m_is_unique) ensure_unique();
    return row(this, i);
  }

  /**
   * Ensures that this is a unique copy
   */
  void ensure_unique();

  /**
   * Modifies the SFrame Rows inplace to enforce typing.
   * \see type_check
   */
  void type_check_inplace(const std::vector<flex_type_enum>& typelist);

  /**
   * Returns a new sframe rows where each column has the set of types enforced
   * \see type_check_inplace
   */
  sframe_rows type_check(const std::vector<flex_type_enum>& typelist) const;

   private:
    std::vector<ptr_to_decoded_column_type> m_decoded_columns;
    mutable bool m_is_unique = true;
  };  // class sframe_rows

/// \}
} // namespace turi
#endif
