/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_SARRAY_V2_BLOCK_MANAGER_HPP
#define TURI_SFRAME_SARRAY_V2_BLOCK_MANAGER_HPP
#include <stdint.h>
#include <vector>
#include <fstream>
#include <tuple>
#include <core/parallel/pthread_tools.hpp>
#include <core/parallel/atomic.hpp>
#include <core/storage/fileio/general_fstream.hpp>
#include <core/storage/sframe_data/sarray_index_file.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/util/buffer_pool.hpp>
#include <core/storage/sframe_data/sarray_v2_block_types.hpp>
#include <core/storage/sframe_data/sarray_v2_type_encoding.hpp>


// forward declaration for LZ4. required here annoyingly since I have a template
// function here which calls it
extern "C" {
  int LZ4_decompress_safe (const char* source, char* dest, int inputSize, int maxOutputSize);
}
namespace turi {


/**
 * \internal
 * \ingroup sframe_physical
 * \addtogroup sframe_internal SFrame Internal
 * \{
 */

/**
 * SFrame v2 Format Implementation Detail
 */
namespace v2_block_impl {

/**
 * Provides block reading capability in v2 segment files.
 *
 * This class manages block reading of an SArray/SArray group , and provides
 * functions to query the blocks (such has how many blocks are there in the
 * segment, and how many rows are there in the block etc).
 *
 * Array Group
 * -----------
 * An array group is a collection of segment files which contain and represent
 * and collection of arrays (columns).
 *
 * Essentially an Array Group comprises of the following:
 * - group.sidx
 *     the group index file. A JSON serialized contents of
 *     group_index_file_information. Describes a collection of arrays.
 * - group.0000, group.0001, group.0002
 *     Each file is one segment of the array group. (multiple segments in an
 *     array group really exist only for parallel writing (and appending)
 *     capabilities. On reading, the segment layout is inconsequential, and a
 *     logical partitioning across threads is used.)
 *
 * Each segment file internally then has the following layout
 *  (1) Consecutive Block contents, each block 4K aligned.
 *  (2) A direct serialization of a vector<vector<block_info> > (blocks[column_id][block_id])
 *  (3) 8 bytes containing the file offset. at which (2) begins
 *
 * For instance, if there are 2 segments with 3 columns each of 20 rows,
 * we may get the following layout:
 *
 * group.0001:
 *   - column 0, block 0, 3 rows
 *   - column 1, block 0, 3 rows
 *   - column 2, block 0, 4 rows
 *   - column 0, block 1, 4 rows
 *   - column 0, block 2, 3 rows
 *   - column 1, block 1, 7 rows
 *   - column 2, block 1, 6 rows
 *
 * group.0002:
 *   - column 1, block 0, 5 rows
 *   - column 0, block 0, 10 rows
 *   - column 1, block 1, 5 rows
 *   - column 2, block 0, 10 rows
 *
 * Observe the following:
 *  1) Each segment contains the same number
 *     of rows from each column. (technically the format does not require
 *     this, but the writer will always produce this result)
 *  2) Blocks can be of different sizes. (the block_manager and block_writer
 *     do not have a block size constraint. The \ref sarray_group_format_writer_v2
 *     tries to keep to a block size of SFRAME_DEFAULT_BLOCK_SIZE
 *     after compression, but this is done by performing block size estimation
 *     (#bytes written / #rows written). But the format itself does not care.
 *  3) Blocks can be laid out in arbitrary order across columns. Striping of
 *     columns is unnecessary)
 *  4) Within each segment, the blocks for a given column are consecutive.
 *
 * File Addressing
 * ---------------
 * Since an array group (and hence a segment) can contain multiple columns,
 * we need a uniform way of addressing a particular column inside an array
 * group, or inside a segment. Thus the following convention is used:
 *
 * Given an array group of 3 columns comprising of the files:
 *  - group.sidx
 *  - group.0000, group.0001, group.0002, group.0003
 *
 * Column 0 in the array group can be addressed by opening the index file
 * "group.sidx:0". Similarly, column 2 can be addressed using "group.sidx:2"
 *
 * Column 2 of the array group thus has the segment files:
 *  - group.0000:2, group.0001:2, group.0002:2, group.0002:3
 *
 * By convention if "group.sidx" is opened as a single array, it refers to
 * column 0.
 *
 * Block Manager
 * -------------
 * The block manager is a singleton reader object that provides read access
 * to columns. The usage convention is:
 *  - block_manager& manager = block_manager::get_instance()
 *  - column_address = manager.open_column("group.0000:2") // opens column 2 in segment
 *  - .. do stuff ..
 *  - manager.close_column(column_address)
 * We will expand on .. do stuff .. below.
 *
 * The reason for having a singleton block manager is to provide better control
 * over file handle utilization. Specifically, the block manager maintains a
 * pool of file handles and will recycle file handles (close them until they
 * are next needed, then reopen and seek) so as to avoid file handle usage
 * exceeding a certain limit (as defined in DEFAULT_FILE_HANDLE_POOL_SIZE)
 * Furthermore, the block manager can combine accesses of multiple columns in the
 * same array group into a single file handle. Future performance improvements
 * involving better IO scheduling can also be performed here.
 *
 * When a column is opened by \ref open_column(), a \ref column_address is
 * returned. This is a pair of integers of {segment_file_id, and column_id}.
 * column_id is the column within the segment. For instance, opening "group.0000:2"
 * will have column_id = 2. The segment_file_id is an internal ID assigned by
 * the block manager to track all accesses to the file group.0000. All open calls
 * to group.0000 will return the same segment_file_id, and a reference counter
 * is used internally to figure out when the file handle and block metadata
 * can be released. \ref close_column() thus must be called for every call
 * to \ref open_column().
 *
 * Once the column is opened, \ref num_blocks_in_column() can be used to obtain
 * the number of blocks in the segment file belonging to the column.
 * \ref read_block() or \ref read_typed_block() can then be used to read the
 * blocks. These functions take a \ref block_address, which is a triple of
 * {segment_file_id, column_id, block_id}. The first 2 fields can be copied
 * from the column_address, the block_id is a sequential counter from 0 to
 * \ref num_blocks_in_column() - 1.
 *
 */
class block_manager {
 public:

  /// Get singleton instance
  static block_manager& get_instance();

  /// default constructor.
  block_manager();

  /**
   * Opens a file of the form segment_file:column_number and returns the
   * the column address: {segment_file_id, column_id}.
   *
   * calling num_blocks_in_column() will return the number of blocks within
   * this column, after which columns can be read by providing
   * {segment_file_id, column_id, block_id} to read_block()
   *
   * close_column() must be called for each call to open_column()
   */
  column_address open_column(std::string column_file);

  /**
   * Releases the column opened with \ref open_column()
   */
  void close_column(column_address addr);

  /**
   * Returns the number of blocks in this column of this segment.
   */
  size_t num_blocks_in_column(column_address addr);

  /** Returns the number of rows in a block
   * Returns (size_t)(-1) on failure.
   */
  const block_info& get_block_info(block_address addr);

  /**
   * Returns all the blockinfo in a segment
   */
  const std::vector<std::vector<block_info> >& get_all_block_info(size_t segment_id);

  /**
   * Reads a block as bytes a block address ((array_group ID, segment ID, block
   * ID) tuple),
   *
   *  If info is not NULL, A pointer to the block information will be stored
   *  info *info. This is a pointer into internal datastructures of the
   *  block manager and should not be modified or freed.
   *
   *  Return an empty pointer on failure.
   *
   *  Safe for concurrent operation.
   */
  std::shared_ptr<std::vector<char> >
    read_block(block_address addr, block_info** ret_info = NULL);


  /**
   * Reads a block given a block address ((array_group ID, segment ID, block
   * ID) tuple), into a typed array. The block must have been stored as
   * a typed block. Returns true on success, false on failure.
   *
   * Safe for concurrent operation.
   */
  bool read_typed_block(block_address addr,
                        std::vector<flexible_type>& ret,
                        block_info** ret_info = NULL);

  /**
   * Reads a few blocks starting from a given a block address ((array_group ID,
   * segment ID, block ID) tuple), into a typed array. The block must have been
   * stored as a typed block. Returns true on success, false on failure.
   *
   * May return less than nblocks if addr goes past the last block.
   *
   * Safe for concurrent operation.
   */
  bool read_typed_blocks(block_address addr,
                         size_t nblocks,
                         std::vector<std::vector<flexible_type> >& ret,
                         std::vector<block_info>* ret_info = NULL);

  /**
   * Reads a few blocks starting from a given a block address ((array_group ID,
   * segment ID, block ID) tuple) and deserializes it into an array. The block
   * Returns true on success, false on failure.
   *
   * May return less than nblocks if addr goes past the last block.
   *
   * Safe for concurrent operation.
   */
  template <typename T>
  bool read_block(block_address addr,
                  std::vector<T>& ret,
                  block_info** ret_info = NULL) {
    bool success = false;
    auto buffer = read_block(addr, ret_info);
    if (buffer) {
      turi::iarchive iarc(buffer->data(), buffer->size());
      iarc >> ret;
      success = true;
    }
    m_buffer_pool.release_buffer(std::move(buffer));
    return success;
  }

 private:

  mutable turi::mutex m_global_lock;
  mutable turi::mutex m_file_handles_lock;
  /**
   * Describes an array group and all the file handles pointing into
   * the array group
   */
  struct segment {
    turi::mutex lock;

    std::string segment_file;

    size_t file_size = 0;

    size_t io_parallelism_id = 0;
    /**
     * File handle to this segment
     */
    std::weak_ptr<general_ifstream> segment_file_handle;

    bool inited = false;

    /** for for each column in the segment, the collection of blocks.
     * Once inited, this array is never modified and is safe for concurrent
     * reads.
     * blocks[column_id][block_id]
     */
    std::vector<std::vector<block_info> > blocks;

    turi::atomic<size_t> reference_count;
  };

  /// All the internal segments
  size_t segment_id_counter = 0;
  std::map<size_t, std::shared_ptr<segment> > m_segments;
  std::map<std::string, size_t> m_file_to_segments;

  /**
   * file handle pool management. We implement a simple LIFO pool.
   */
  std::deque<std::weak_ptr<general_ifstream> > m_file_handle_pool;

  /// Pool of buffers used for decompression, returns, etc.
  buffer_pool<std::vector<char> > m_buffer_pool;

/**************************************************************************/
/*                                                                        */
/*                           Private Functions                            */
/*                                                                        */
/**************************************************************************/

  /// Returns a new file handle from the file handle pool
  std::shared_ptr<general_ifstream> get_new_file_handle(std::string file);

  /**
   * Returns an opened handle to a segment file in an array group.
   * Handle may be pointing anywhere within the file. This will reuse an
   * existing handle if the handle has not yet been collected, and will
   * create a new handle if none exist.
   * Locks are not acquired and it is up to the caller to ensure locking.
   */
  std::shared_ptr<general_ifstream>
      get_segment_file_handle(std::shared_ptr<segment>& group);

  /**
   * reads a block from an input stream.
   * Decompresses the block if it was compressed.
   * Returns false on failure.
   */
  bool read_block_from_stream(general_ifstream& fin, std::vector<char>& ret,
                              block_info& info);

  std::shared_ptr<segment> get_segment(size_t segmentid);

  void init_segment(std::shared_ptr<segment>& seg);
};

} // namespace v2_block_impl

/// \}
} // namespace turi
#endif
