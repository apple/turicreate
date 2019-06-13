/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_HDFS_HPP
#define TURI_HDFS_HPP

// Requires the hdfs library
#ifdef HAS_HADOOP
extern "C" {
  #include <hdfs.h>

// Define structs not defined in hdfs.h

  /*
   * The C equivalent of org.apache.org.hadoop.FSData(Input|Output)Stream .
   */
  enum hdfsStreamType
  {
    HDFS_STREAMTYPE_UNINITIALIZED = 0,
    HDFS_STREAMTYPE_INPUT = 1,
    HDFS_STREAMTYPE_OUTPUT = 2,
  };

  /**
   * The 'file-handle' to a file in hdfs.
   */
  struct hdfsFile_internal {
    void* file;
    enum hdfsStreamType type;
    int flags;
  };

}
#endif

#include <vector>
#include <iostream>
#include <boost/iostreams/stream.hpp>
#include <core/logging/assertions.hpp>


namespace turi {

#ifdef HAS_HADOOP
  /**
   * \ingroup fileio
   * \internal
   * Wrapper around libHDFS
   */
  class hdfs {
  private:
    /** the primary filesystem object */
    hdfsFS filesystem;
  public:
    /** hdfs file source is used to construct boost iostreams */
    class hdfs_device {
    public: // boost iostream concepts
      typedef char                                          char_type;
      struct category :
        public boost::iostreams::device_tag,
        public boost::iostreams::multichar_tag,
        public boost::iostreams::closable_tag,
        public boost::iostreams::bidirectional_seekable { };
        // while this claims to be bidirectional_seekable, that is not true
        // it is only read seekable. Will fail when seeking on write
    private:
      hdfsFS filesystem;

      hdfsFile file;

      size_t m_file_size;

    public:
      hdfs_device() : filesystem(NULL), file(NULL) { }

      hdfs_device(const hdfs& hdfs_fs, const std::string& filename, const bool write = false);

      //      ~hdfs_device() { if(file != NULL) close(); }

      // Because the device has bidirectional tag, close will be called
      // twice, one with the std::ios_base::in, followed by out.
      // Only close the file when the close tag matches the actual file type.
      void close(std::ios_base::openmode mode = std::ios_base::openmode());

      /** the optimal buffer size is 0. */
      inline std::streamsize optimal_buffer_size() const { return 0; }

      std::streamsize read(char* strm_ptr, std::streamsize n);

      std::streamsize write(const char* strm_ptr, std::streamsize n);

      bool good() const { return file != NULL; }

      /**
       * Seeks to a different location.
       */
      std::streampos seek(std::streamoff off,
                          std::ios_base::seekdir way,
                          std::ios_base::openmode);
    }; // end of hdfs device

    /**
     * The basic file type has constructor matching the hdfs device.
     */
    typedef boost::iostreams::stream<hdfs_device> fstream;

    /**
     * Open a connection to the filesystem. The default arguments
     * should be sufficient for most uses
     */
    hdfs(const std::string& host = "default", tPort port = 0);

    bool good() const { return filesystem != NULL; }

    ~hdfs() {
      if (good()) {
        const int error = hdfsDisconnect(filesystem);
        ASSERT_EQ(error, 0);
      }
    } // end of ~hdfs

    /**
     * Returns the contents of a directory
     */
    std::vector<std::string> list_files(const std::string& path) const;

    /**
     * Returns the contents of a directory as well as a boolean for every
     * file identifying whether the file is a directory or not.
     */
    std::vector<std::pair<std::string, bool> > list_files_and_stat(const std::string& path) const;

    /**
     * Returns the size of a given file. Returns (size_t)(-1) on failure.
     */
    size_t file_size(const std::string& path) const;

    /**
     * Returns true if the given path exists
     */
    bool path_exists(const std::string& path) const;

    /**
     * Returns true if the given path is a directory, false if it
     * does not exist, or if is a regular file
     */
    bool is_directory(const std::string& path) const;


    /**
     * Creates a subdirectory and all parent required directories (like mkdir -p)
     * Returns true on success, false on failure.
     */
    bool create_directories(const std::string& path) const;

    /**
     * Change the permissions of the file.
     */
    bool chmod(const std::string& path, short mode) const;


    /**
     * Deletes a single file / empty directory.
     * Returns true on success, false on failure.
     */
    bool delete_file_recursive(const std::string& path) const;

    inline static bool has_hadoop() { return true; }

    static hdfs& get_hdfs();

    static hdfs& get_hdfs(std::string host, size_t port);
  }; // end of class hdfs
#else

  class hdfs {
  public:
    /** hdfs file source is used to construct boost iostreams */
    class hdfs_device {
    public: // boost iostream concepts
      typedef char                                          char_type;
      typedef boost::iostreams::bidirectional_device_tag    category;
    public:
      hdfs_device(const hdfs& hdfs_fs, const std::string& filename,
                  const bool write = false) {
        logstream(LOG_FATAL) << "Libhdfs is not installed on this system."
                             << std::endl;
      }
      void close() { }
      std::streamsize read(char* strm_ptr, std::streamsize n) {
        logstream(LOG_FATAL) << "Libhdfs is not installed on this system."
                             << std::endl;
        return 0;
      } // end of read
      std::streamsize write(const char* strm_ptr, std::streamsize n) {
        logstream(LOG_FATAL) << "Libhdfs is not installed on this system."
                             << std::endl;
        return 0;
      }
      bool good() const { return false; }

    }; // end of hdfs device

    /**
     * The basic file type has constructor matching the hdfs device.
     */
    typedef boost::iostreams::stream<hdfs_device> fstream;

    /**
     * Open a connection to the filesystem. The default arguments
     * should be sufficient for most uses
     */
    hdfs(const std::string& host = "default", int port = 0) {
      logstream(LOG_FATAL) << "Libhdfs is not installed on this system."
                           << std::endl;
    } // end of constructor

    inline std::vector<std::string> list_files(const std::string& path) const {
      logstream(LOG_FATAL) << "Libhdfs is not installed on this system."
                           << std::endl;
      return std::vector<std::string>();;
    } // end of list_files

    inline std::vector<std::pair<std::string, bool> > list_files_and_stat(const std::string& path) const {
      logstream(LOG_FATAL) << "Libhdfs is not installed on this system."
                           << std::endl;
      return std::vector<std::pair<std::string, bool>>();
    }

    inline size_t file_size(const std::string& path) const {
      return (size_t)(-1);
    }

    /**
     * Returns true if the given path exists
     */
    inline bool path_exists(const std::string& path) const {
      return false;
    }

    inline bool is_directory(const std::string& path) const {
      return false;
    }

    bool create_directories(const std::string& path) const {
      return false;
    }

    bool delete_file_recursive(const std::string& path) const {
      return false;
    }

    bool good() const { return false; }

    // No hadoop available
    inline static bool has_hadoop() { return false; }

    static hdfs& get_hdfs();

    static hdfs& get_hdfs(std::string host, size_t port);
  }; // end of class hdfs


#endif

}; // end of namespace turi
#endif
