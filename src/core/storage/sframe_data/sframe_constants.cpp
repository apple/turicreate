/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sframe_data/sframe_constants.hpp>
#include <core/globals/globals.hpp>
#include <limits>
#include <core/export.hpp>
namespace turi {

// will be modified at startup to match the number of CPUs
EXPORT size_t SFRAME_DEFAULT_NUM_SEGMENTS = 16;
EXPORT const size_t DEFAULT_SARRAY_READER_BUFFER_SIZE = 1024;
EXPORT const size_t SARRAY_FROM_FILE_BATCH_SIZE = 32768;
EXPORT const size_t MIN_SEGMENT_LENGTH = 1024;
EXPORT const size_t SFRAME_WRITER_BUFFER_SOFT_LIMIT = 1024 * 4;
EXPORT const size_t SFRAME_WRITER_BUFFER_HARD_LIMIT = 1024 * 10;
EXPORT size_t SFRAME_FILE_HANDLE_POOL_SIZE = 128;
EXPORT const size_t SFRAME_BLOCK_MANAGER_BLOCK_BUFFER_COUNT = 128;
EXPORT const float COMPRESSION_DISABLE_THRESHOLD = 0.9;
EXPORT size_t SFRAME_DEFAULT_BLOCK_SIZE =  64 * 1024;
EXPORT const size_t SARRAY_WRITER_MIN_ELEMENTS_PER_BLOCK = 8;
EXPORT const size_t SARRAY_WRITER_INITAL_ELEMENTS_PER_BLOCK = 16;
EXPORT size_t SFRAME_WRITER_MAX_BUFFERED_CELLS = 32*1024*1024; // 64M elements
EXPORT size_t SFRAME_WRITER_MAX_BUFFERED_CELLS_PER_BLOCK = 256*1024; // 1M elements.
EXPORT // will be modified at startup to be 4x nCPUS
EXPORT size_t SFRAME_MAX_BLOCKS_IN_CACHE = 32;
EXPORT size_t SFRAME_CSV_PARSER_READ_SIZE = 50 * 1024 * 1024; // 50MB
EXPORT size_t SFRAME_GROUPBY_BUFFER_NUM_ROWS = 1024 * 1024;
EXPORT size_t SFRAME_JOIN_BUFFER_NUM_CELLS = 50*1024*1024;
EXPORT size_t SFRAME_IO_READ_LOCK = false;
EXPORT size_t SFRAME_SORT_PIVOT_ESTIMATION_SAMPLE_SIZE = 2000000;
EXPORT size_t SFRAME_SORT_MAX_SEGMENTS = 128;
EXPORT const size_t SFRAME_IO_LOCK_FILE_SIZE_THRESHOLD = 4 * 1024 * 1024;
EXPORT size_t SFRAME_COMPACTION_THRESHOLD = 256;
EXPORT size_t FAST_COMPACT_BLOCKS_IN_SMALL_SEGMENT = 8;


REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SFRAME_DEFAULT_NUM_SEGMENTS,
                            true,
                            +[](int64_t val){ return val >= 1; });


REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SFRAME_FILE_HANDLE_POOL_SIZE,
                            true,
                            +[](int64_t val){ return val >= 64; });


REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SFRAME_DEFAULT_BLOCK_SIZE,
                            true,
                            +[](int64_t val){ return val >= 1024; });


REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SFRAME_MAX_BLOCKS_IN_CACHE,
                            true,
                            +[](int64_t val){ return val >= 1; });


REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SFRAME_CSV_PARSER_READ_SIZE,
                            true,
                            +[](int64_t val){ return val >= 1024; });


REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SFRAME_GROUPBY_BUFFER_NUM_ROWS,
                            true,
                            +[](int64_t val){ return val >= 64; });


REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SFRAME_JOIN_BUFFER_NUM_CELLS,
                            true,
                            +[](int64_t val){ return val >= 1024; });



REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SFRAME_WRITER_MAX_BUFFERED_CELLS,
                            true,
                            +[](int64_t val){ return val >= 1024; });


REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SFRAME_WRITER_MAX_BUFFERED_CELLS_PER_BLOCK,
                            true,
                            +[](int64_t val){ return val >= 1024; });

REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SFRAME_IO_READ_LOCK,
                            true,
                            +[](int64_t val){ return val == 0 || val == 1 ; });

REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SFRAME_SORT_PIVOT_ESTIMATION_SAMPLE_SIZE,
                            true,
                            +[](int64_t val){ return val > 128 ; });

REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SFRAME_SORT_MAX_SEGMENTS,
                            true,
                            +[](int64_t val){ return val > 1; });


REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            FAST_COMPACT_BLOCKS_IN_SMALL_SEGMENT,
                            true,
                            +[](int64_t val){ return val >= 1; });


REGISTER_GLOBAL(int64_t,
                SFRAME_COMPACTION_THRESHOLD,
                true);
} // namespace turi
