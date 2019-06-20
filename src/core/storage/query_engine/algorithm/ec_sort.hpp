/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef SFRAME_ALGORITHM_EC_SORT_HPP
#define SFRAME_ALGORITHM_EC_SORT_HPP

#include <vector>
#include <memory>

namespace turi {
class sframe;

namespace query_eval {

struct planner_node;

/**
 * \ingroup sframe_query_engine
 * \addtogroup Algorithms Algorithms
 * \{
 */
/**
 * External Memory Columnar Sort. See \ref ec_sort for details.
 *
 *
 * The current sort algorithm (in \ref turi::query_eval::sort) implementation
 * has lasted us a while and it is time to think about something better.
 *
 * A brief overview of the old sort algorithm
 * ==========================================
 *
 * The old algorithm implemented in \ref sort essentially a bucket sort.
 *
 * Pivot generation
 * ----------------
 *  - A fraction of random elements is selected on the key column filling a
 *  quantile sketch
 *  - The quantile sketch is used to select a set of K-1 pivots (hence K buckets)
 *  - Each bucket is associated with an output file
 *
 * Scatter
 * -------
 *  - The entire sframe is materialized to a stream which consumes the sframe row
 *  by row.
 *  - For each row, the key is compared to the pivots, and the row is written
 *  row-wise into the bucket.
 *  - This process can be done in parallel
 *
 * Sort
 * ----
 *  - Each bucket is loaded into memory and an in memory quicksort is performed.
 *  - This process can be done in parallel
 *
 * Advantages
 * ----------
 *
 * 1. Exactly 2x write amplification. Every tuple is only written exactly twice.
 * (But see Issue 1)
 * 2. Works with Lazy sframe sources
 *
 * Issues
 * ------
 *
 * 1. Much more than 2x write amplification.Though the buckets are not that well
 * compressed since they are done row-wise. So while every tuple is written
 * exactly twice, the effective \#bytes written can be *much much* larger.
 *
 * 2. Wide reads of SFrames are slow. If the SFrame to be sorted has a few hundred
 * columns on disk, things break.
 *
 * 3. Memory limits are hard to control. Image columns or very big dictionaries /
 * lists are problematic.
 *
 * The Proposed algorithm
 * =======================
 * Firstly, we assume that the sframe is read one value at a time, i.e.
 * we get a stream of *(column\_number, row\_number, value)*. With the
 * assumption that the data is at least, sequential within the column.
 * I.e. if I just received *(c, r, v) : r \> 0*, Â  I must have already
 * received *(c, r-1, v)*
 *
 * The the algorithm proceeds as such:
 *
 * Forward Map Generation
 * ----------------------
 *
 * - A set of row numbers are added to the key columns, and the key
 * columns are sorted. And then dropped. This gives the inverse map.
 * (i.e. x[i] = j implies output row i is read from input row j)
 * - Row numbers are added again, and its sorted again by the first set
 * of row numbers. This gives the forward map (i.e. y[i] = j implies
 * input row i is written to output row j)
 * - (In SFrame pseudocode:
 *
 *     B = A[['key']].add_row_number('r1').sort('key')
 *     inverse_map = B['r1'] # we don't need this
 *     C = B.add_row_number('r2').sort('r1')
 *     foward_map = C['r2']
 *
 * The forward map is held as an SArray of integers.
 *
 * Pivot Generation
 * ----------------
 * - Now we have a forward map, we can get exact buckets. Of N/K
 * length. I.e. row r is written to bucket `Floor(K \ forward_map(r) / N)`
 *
 * Scatter
 * -------
 *  - For each (c,r,v) in data:
 *    Write (c,v) to bucket `Floor(K \ forward_map(r) / N)`
 *
 * This requires a little modification to the sframe writer to allow
 * single columns writes (really it already does this internally. It
 * transposes written rows to turn it to columns). This exploits the
 * property that if (c,r-1,v) must be read before (c,r,v). Hence the
 * rows are written in the correct order. (though how to do this in
 * parallel is a question.)
 *
 * We will also have to generate a per-bucket forward_map using the same
 * scatter procedure.
 *
 * This requires a little bit of intelligence in the caching of the
 * forward map SArray. If the forward map is small, we can keep it all
 * in memory. If it is large, need a bit more work. Some intelligence
 * needed in this datastructure.
 *
 * Sort
 * ----
 *
 *     For each Bucket b:
 *         Allocate Output vector of (Length of bucket) * (#columns)
 *         Let S be the starting index of bucket b (i.e. b*N/k)
 *         Let T be the ending index of bucket b (i.e. (b+1)*N/k)
 *         Load forward_map[S:T] into memory
 *         For each (c,r,v) in bucket b
 *             Output[per_bucket_forward_map(r) - S][c] = v
 *         Dump Output to an SFrame
 *
 * Advantages
 * ----------
 *
 * 1. Only sequential sweeps on the input SFrames, block-wise.
 * 2. Does not matter if there are a large number of columns.
 * 3. Memory limits can be easier to control.
 *   a.  Scatter has little memory requirements (apart from the write buffer stage).
 *   b.  The forward map is all integers.
 *   c.  The Sort stage can happen a few columns at a time.
 *
 * Issues
 * ------
 *
 * 1. The block-wise reads do not work well on lazy SFrames. In theory this is
 * possible, since the algorithm is technically more general and will work even if
 * the (c,r,v) tuples are generated one row at a time (i.e. wide reads). However,
 * executing this on a lazy Sframe, means that we *actually* perform the wide
 * reads which are slow.  (Example: if I have a really wide SFrame on disk.
 * And I perform the read through the query execution pipeline, the query
 * execution pipeline performs the wide read and that is slow.  Whereas if I go to
 * the disk SFrame directly, I can completely avoid the wide read).
 *
 *
 * 2. Due to (1) it is better to materialize the SFrame and operate on
 * the physical SFrame. Hence we get up to 3x write amplification.
 * However, every intermediate bucket is fully compressed.
 *
 * Optimizations And implementation Details
 * ----------------------------------------
 *
 * - One key aspect is the construction of the forward map. We still
 * need to have a second sort algorithm to make that work. We could use
 * our current sorting algo, or try to make something simpler since the
 * 'value' is always a single integer. As a start, I would just use our
 * current sort implementation. And look for something better in the
 * future.
 *
 * - We really need almost random seeks on the forward map. I.e. it is mostly
 * sequential due to the column ordering. Some intelligence is needed to decide
 * how much of the forward map to hold in memory, and how much to keep on disk.
 *
 * - Particular excessively heavy columns (like super big dictionaries,
 * lists, images) , we store the row number instead during the scatter
 * phase. And we use seeks against the original input to transfer it
 * straight from input to output. We could take a guess about the
 * column data size and use the seek strategy if it is too large.
 *
 * - There are 2 optimizations possible.
 *    - One is the above: we write row number into the bucket, and we go back
 *    to the original input SFrame for data. We can put a threshold size here
 *    of say ... 256 KB. about 100MBps * 2ms.
 *
 *    - The second is optimization in the final sort of the bucket. We can perform
 *    seeks to fetch elements from the bucket. Unlike the above, we pay for an
 *    additional write + seek of the element, but it IS a smaller file.
 *
 * - Minimize full decode of dictionary/list type. We should be able to
 * read/write dict/list columns in the input SFrame as strings. (Annoyingly
 * this is actually quite hard to achieve at the moment)
 *
 * Notes on the current implementation
 * -----------------------------------
 * - The current implementation uses the old sort algorithm for the backward
 *   map generation, and the forward map generation. A faster algorithm could be
 *   used for the forward map sort since it now very small. A dedicated sort
 *   around just sorting integers could be nice.
 *
 * \param sframe_planner_node The lazy sframe to be sorted
 * \param sort_column_names The columns to be sorted
 * \param sort_orders The order for each column to be sorted, true is ascending
 *
 * \return The sorted sframe
 */
std::shared_ptr<sframe> ec_sort(
    std::shared_ptr<planner_node> sframe_planner_node,
    const std::vector<std::string> column_names,
    const std::vector<size_t>& key_column_indices,
    const std::vector<bool>& sort_orders);

/// \}
} // query_eval
} // turicreate
#endif
