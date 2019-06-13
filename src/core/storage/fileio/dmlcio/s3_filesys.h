/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
/*!
 *  Copyright (c) 2015 by Contributors
 * \file s3_filesys.h
 * \brief S3 access module
 * \author Tianqi Chen
 */
#ifndef DMLC_IO_S3_FILESYS_H_
#define DMLC_IO_S3_FILESYS_H_
#include <string>
#include <vector>
#include "./io.h"
#include "./filesys.h"

namespace dmlc {
namespace io {
/*! \brief AWS S3 filesystem */
class S3FileSystem {
 public:
  /*! \brief constructor */
  S3FileSystem() = default;

  void SetCredentials(const std::string& aws_access_id,
                      const std::string& aws_secret_key);

  /*! \brief destructor */
  virtual ~S3FileSystem() {}
  /*!
   * \brief get information about a path
   * \param path the path to the file
   * \return the information about the file
   */
  virtual FileInfo GetPathInfo(const URI &path);
  /*!
   * \brief list files in a directory
   * \param path to the file
   * \param out_list the output information about the files
   */
  virtual void ListDirectory(const URI &path, std::vector<FileInfo> *out_list);
  /*!
   * \brief open a stream, will report error and exit if bad thing happens
   * NOTE: the Stream can continue to work even when filesystem was destructed
   * \param path path to file
   * \param uri the uri of the input
   * \param flag can be "w", "r", "a"
   * \return the created stream, can be NULL when allow_null == true and file do not exist
   */
  virtual Stream *Open(const URI &path, const char* const flag);
  /*!
   * \brief open a seekable stream for read
   * \param path the path to the file
   * \return the created stream, can be NULL
   */
  virtual SeekStream *OpenForRead(const URI &path);
  /*!
   * \brief get a singleton of S3FileSystem when needed
   * \return a singleton instance
   */
  inline static S3FileSystem *GetInstance(void) {
    static S3FileSystem instance;
    return &instance;
  }

 private:
  /*! \brief AWS access id */
  std::string aws_access_id_;
  /*! \brief AWS secret key */
  std::string aws_secret_key_;
  /*!
   * \brief try to get information about a path
   * \param path the path to the file
   * \param out_info holds the path info
   * \return return false when path do not exist
   */
  bool TryGetPathInfo(const URI &path, FileInfo *info);
};
}  // namespace io
}  // namespace dmlc
#endif
