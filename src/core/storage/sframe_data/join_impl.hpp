/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cstdio>
#include <unordered_set>
#include <unordered_map>

#include <core/storage/sframe_data/sframe.hpp>

//TODO: What happens if a join key (or part of one) is NULL?
enum join_type_t {INNER_JOIN = 0, LEFT_JOIN, RIGHT_JOIN, FULL_JOIN};

namespace turi {


/**
 * \ingroup sframe_physical
 * \addtogroup Join Join
 * \{
 */

/**
 * Join Implementation Detail
 */
namespace join_impl {

size_t hash_key(const std::vector<flexible_type>& key);
size_t compute_hash_from_row(const std::vector<flexible_type> &row,
                             const std::vector<size_t> &positions);

typedef struct {
  std::vector<std::vector<flexible_type>> rows;
  bool matched;
} hash_join_row_t;

/**
 * This class is the keeper of an in-memory hash table for use in a join
 * algorithm. Its methods facilatate hashing by given join keys by taking
 * a vector of positions these keys are in a row.
 *
 * NOTE:Could be templated for hash_type, row_type, but I don't see a need now.
 */
class join_hash_table {
 public:
  typedef hash_join_row_t value_type;

  /**
   * Constructor.  Takes a vector of hash positions, which are the column
   * numbers in each row that represent the values the join is on (or the join
   * keys).  These hash positions are for the frame that each row is added from.
   */
  join_hash_table(std::vector<size_t> hp) : _hash_positions(hp) {}

  /**
   * Add a row to the hash table.  Each row must be from the same frame, or
   * else join results will not make sense.
   */
  bool add_row(const std::vector<flexible_type> &row);

  /**
   * Returns all rows whose join keys match the given row's join keys.
   *
   * The return object is a hash_join_row_t, a structure containing a list of
   * rows and whether this join key was "matched" or not.  An optional argument
   * marks all of the matching rows as "matched", which is usually used for
   * completing a left join, in deciding which rows need to be joined with NULL
   * values and emitted into the result set.
   */
  const value_type &get_matching_rows(const std::vector<flexible_type> &row,
                                      const std::vector<size_t> &hash_positions,
                                      bool mark_match=true);

  /**
   * Prints stats about the hash table to the log.
   */
  size_t num_stored_rows();
  std::unordered_map<size_t, std::list<value_type>>::const_iterator cbegin();
  std::unordered_map<size_t, std::list<value_type>>::const_iterator cend();


 private:
  /**
   * Does an itemwise check to see if two rows have matching join keys.
   */
  bool join_values_equal(const std::vector<flexible_type> &row,
      const std::vector<flexible_type> &other,
      const std::vector<size_t> &hash_positions);

  // Holds a list of each row with an identical hash key.  If we are joining on
  // a key that holds a unique primary key within it, all entries will have a vector
  // with 1 element.  We can't guarantee this though, so the value type is a vector
  // of vectors.
  std::unordered_map<size_t, std::list<value_type>> _hash_table;
  // The positions in the rows that we store taht make up the hash key
  std::vector<size_t> _hash_positions;
  value_type empty_vt;
};

/**
 * The hash_join_executor class executes a hash join.  It is only meant
 * to perform one join.  Could eventually inherit from a generic join class
 * and have further OOP design patterns to choose the algorithm (or just
 * different function calls).  Only one algorithm implemented thus far,
 * so no need for that yet.
 */
class hash_join_executor {
 public:
  //TODO: Perhaps combine the sframe and the join positions into a struct?
  hash_join_executor(const sframe &left,
                     const sframe &right,
                     const std::vector<size_t> &left_join_positions,
                     const std::vector<size_t> &right_join_positions,
                     join_type_t join_type,
                     size_t max_buffer_size);

  ~hash_join_executor() {}

  sframe grace_hash_join();

 private:
  // The original frames we were passed
  sframe _left_frame;
  sframe _right_frame;
  std::vector<size_t> _left_join_positions;
  std::vector<size_t> _right_join_positions;
  size_t _max_buffer_size;
  bool _left_join;
  bool _right_join;
  std::unordered_map<size_t,size_t> _right_to_left_join_positions;
  bool _reverse_output_column_order;
  std::unordered_map<size_t, std::string> _changed_dup_names;
  bool _frames_partitioned;

  /**
   * Partition the left and right frames for the GRACE hash join algorithm and
   * write these partitions out to disk.
   */
  std::pair<std::shared_ptr<sframe>,std::shared_ptr<sframe>> grace_partition_frames();

  /**
   * Partition one SFrame for the GRACE hash join algorithm.
   *
   * Used by grace_partition_frames().
   */
  std::shared_ptr<sframe> grace_partition_frame(const sframe &sf, const std::vector<size_t> &join_col_nums, size_t num_partitions);

  /**
   * Return the number of cells (rows * cols) of an sframe.
   */
  size_t get_num_cells(const sframe &sf);

  /**
   * Estimates how many partitions this SFrame should be divided into for the
   * GRACE hash join. The goal is for each partition to fit into memory.
   *
   */
  size_t choose_number_of_grace_partitions(const sframe &sf);

  /**
   * Create an empty SFrame that includes the columns of both left and right
   * frames without duplicating the join columns.
   */
  void init_result_frame(sframe &result_frame);

  /**
   * Join a vector of rows from the left frame with a vector of rows from the
   * right frame and write to the given output iterator.
   *
   * If both vectors have size > 0, this will perform a cross product and write
   * the result. If one vector is empty and the other is not, each row from the
   * non-empty vector is join with 'NULL' values, making sure that each join
   * column is not 'NULL'.
   */
  void merge_rows_for_output(sframe &result_frame,
                             sframe::iterator result_iter,
                             const std::vector<std::vector<flexible_type>> &left_rows,
                             const std::vector<std::vector<flexible_type>> &right_rows);

  std::vector<flexible_type> unpack_row(std::string val, size_t num_cols);
};

} // end of join_impl

/// \}
} // end of turicreate
