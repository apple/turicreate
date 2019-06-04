/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SERIALIZATION_DIR_ARCHIVE_HPP
#define TURI_SERIALIZATION_DIR_ARCHIVE_HPP
#include <cstddef>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <core/storage/fileio/fs_utils.hpp>
#include <core/storage/fileio/general_fstream.hpp>
namespace turi {

/**
 * This file is the human readable INI file in the directory containing
 * information about the archive.
 */
extern const char* DIR_ARCHIVE_INI_FILE;

/**
 * This file is the binary archive used to hold serializable object.
 */
extern const char* DIR_ARCHIVE_OBJECTS_BIN;



namespace dir_archive_impl {

/**
 * The archive index.
 *
 * The archive index file simply comprises of the following:
 * \code
 * [archive]
 * version = 1
 * num_prefixes = 4
 * [prefixes]
 * 0000 = "dir_archive.ini"
 * 0001 = "objects.bin"
 * 0002 = "0001"
 * 0003 = "0002"
 * \endcode
 * The prefix section basically lists all the prefixes stored inside the
 * directory archive. All files in the directory which have their file name
 * beginning with a prefix is a file belonging to the archive.
 *
 * The objects.bin, and dir_archive.ini file is always in the prefix
 *
 * Once read into the archive_index_information struct however, the prefixes
 * will all be absolute paths.
 */
struct archive_index_information {
  size_t version = (size_t)(-1);
  std::vector<std::string> prefixes;
  std::map<std::string, std::string> metadata;
};

} // namespace dir_archive_impl


/**
 * \ingroup group_serialization
 * The dir_archive object manages a directory archive. It is an internal
 * class which provides two basic containers:
 *  - A single file stream object (a general_ifstream / general_ofstream)
 *    which points to an "objects.bin" file in the directory.
 *  - The ability to obtain prefixes (for instance [directory]/0000) which
 *    consumers can then use for other file storage purposes. (for instance,
 *    an sframe could create 0000.sidx, 0000.0001, 0000.0002, etc),
 *
 * The directory archive provide management for the prefixes and the objects
 * as well as directory archive creation / deletion.
 *
 * To use:
 * \code
 * dir_archive archive;
 * archive.open_directory_for_write(dir)
 * oarchive oarc(archive)
 * oarc << ...
 * oarc.get_prefix()
 * etc.
 * \endcode
 * Similarly, to read:
 * \code
 * dir_archive archive;
 * archive.open_directory_for_read(dir)
 * iarchive iarc(archive)
 * iarc >> ...
 * iarc.get_prefix()
 * etc.
 * \endcode
 */
class dir_archive {
 public:
  inline dir_archive() { }

  /**
   * Destructor. Also closes.
   */
  ~dir_archive();

  /**
   * Opens a directory for writing. Directory must be an absolute path.
   *
   * if fail_on_existing is false: (default)
   *  - This function will only fail if the directory exists, and does not
   *  contain an archive. It will overwrite in all other cases.
   *
   * if fail_on_existing is true:
   *  - The function will fail if the the directory points to a file name.
   *  - The function will fail if the the directory exists.
   *
   * Throws an exception with a string message if the directory cannot
   * be opened.
   */
  void open_directory_for_write(std::string directory,
                                bool fail_on_existing = false);

  /**
   * Opens a directory for reading. Directory must be an absolute path.
   * This function will fail if the directory is not an archive.
   *
   * Throws an exception with a string message if the directory cannot
   * be opened.
   */
  void open_directory_for_read(std::string directory);

  /**
   * Returns the current directory opened by either
   * open_directory_for_read() or open_directory_for_write();
   * if nothing is opened, this returns an empty string.
   */
  std::string get_directory() const;

  /**
   * The directory must be opened for write.
   * This returns a new prefix which can be written to.
   */
  std::string get_next_write_prefix();

  /**
   * The directory must be opened for read.
   * This returns the next prefix in the sequence of generated prefixes.
   * The order of prefixes returns is the same order as the prefixes generated
   * by get_next_write_prefix() when the archive was created.
   */
  std::string get_next_read_prefix();
  /**
   * Returns a pointer to the object stream reader. Returns NULL if the
   * input directory is not opened for read.
   */
  general_ifstream* get_input_stream();
  /**
   * Returns a pointer to the object stream writer. Returns NULL if the
   * input directory is not opened for write.
   */
  general_ofstream* get_output_stream();

  /**
   * Closes the directory archive, committing all writes.
   */
  void close();

  /**
   * Associates additional metadata with the archive that can be read back
   * with get_metadata() when it is loaded.
   */
  void set_metadata(std::string key, std::string val);

  /**
   * Reads any metadata associated with the archive.
   * Returns true if the key exists, false otherwise.
   */
  bool get_metadata(std::string key, std::string& val) const;

  /**
   * Deletes the contents of an archive safely. (i.e. performing
   * a non-recursive delete so we don't *ever*, even by accident, delete
   * stuff we are not meant to delete).
   *
   * It will delete the directory the archive is in if the directory is empty
   * after deletion of all the archive files.
   *
   * Never throws.
   */
  static void delete_archive(std::string directory);

  /*
   * Given a directory where one Turi object is stored, return the requested
    metadata of the object.
    Could throw if key does not exist or directory does not store a valid Turi
    object
  */
  static std::string get_directory_metadata(
    std::string directory,
    const std::string& key);

  /**
   * Returns true if the directory contains an archive
   */
  static bool directory_has_existing_archive(
      const std::vector<std::pair<std::string, fileio::file_status> >& dircontents);

 private:

  void set_close_callback(std::function<void()>&);

  void init_for_read(const std::string& directory);

  void init_for_write(const std::string& directory);

  void make_s3_read_cache(const std::string& directory);

  /**
   * The index information for the archive
   */
  dir_archive_impl::archive_index_information m_index_info;

  std::string m_directory;

  /**
   * The pointer to the objects.bin write stream
   */
  std::unique_ptr<general_ofstream> m_objects_out;

  /**
   * The pointer to the objects.bin read stream
   */
  std::unique_ptr<general_ifstream> m_objects_in;

  /// The next element in m_index_info.prefixes to return
  size_t m_read_prefix_index = 0;

  /// The next prefix number to return
  size_t m_write_prefix_index = 0;

  /// Cache dir_archive
  std::unique_ptr<dir_archive> m_cache_archive;

  /// callback on close
  std::function<void()> m_close_callback;
};


} // namespace turi
#endif
