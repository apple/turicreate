/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_SAVING_HPP
#define TURI_SFRAME_SAVING_HPP
namespace turi {
class sframe;


/**
 * \ingroup sframe_physical
 * \addtogroup sframe_main Main SFrame Objects
 * \{
 */


/**
 * Saves an SFrame to another index file location using the most naive method:
 * decode rows, and write them
 */
void sframe_save_naive(const sframe& sf,
                       std::string index_file);

/**
 * Saves an SFrame to another index file location using a more efficient method,
 * block by block.
 */
void sframe_save_blockwise(const sframe& sf,
                           std::string index_file);

/**
 * Automatically determines the optimal strategy to save an sframe
 */
void sframe_save(const sframe& sf,
                 std::string index_file);

/**
 * Performs an "incomplete save" to a target index file location.
 * All this ensures is that the sframe's contents are located on the
 * same "file-system" (protocol) as the index file. Essentially the reference
 * save is guaranteed to be valid for only as long as no other SFrame files are
 * deleted.
 *
 * Essentially this can be used to build a "delta" SFrame.
 * - You already have an SFrame on disk somewhere. Say... /data/a
 * - You open it and add a column
 * - Calling sframe_save_weak_reference to save it to /data/b
 * - The saved SFrame in /data/b will include just the new column, but reference
 * /data/a for the remaining columns.
 *
 * \param sf The SFrame to save
 * \param index_file The output file location
 *
 */
void sframe_save_weak_reference(const sframe& sf,
                                std::string index_file);


/// \}
}; // naemspace turicreate

#endif
