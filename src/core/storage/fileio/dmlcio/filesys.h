/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
/*!
 *  Copyright (c) 2015 by Contributors
 * \file filesystem.h
 * \brief general file system io interface
 * \author Tianqi Chen
 */
#ifndef DMLC_IO_FILESYS_H_
#define DMLC_IO_FILESYS_H_

#include <cstring>
#include <string>
#include "io.h"

namespace dmlc {
namespace io {
/*! \brief common data structure for URI */
struct URI {
  /*! \brief protocol */
  std::string protocol;
  /*!
   * \brief host name, namenode for HDFS, bucket name for s3
   */
  std::string host;
  /*! \brief name of the path */
  std::string name;
  /*! \brief enable default constructor */
  URI(void) {}
  /*!
   * \brief construct from URI string
   */
  explicit URI(const char *uri) {
    const char *p = std::strstr(uri, "://");
    if (p == NULL) {
      name = uri;
    } else {
      protocol = std::string(uri, p - uri + 3);
      uri = p + 3;
      p = std::strchr(uri, '/');
      if (p == NULL) {
        host = uri; name = '/';
      } else {
        host = std::string(uri, p - uri);
        name = p;
      }
    }
  }
  /*! \brief string representation */
  inline std::string str(void) const {
    return protocol + host + name;
  }
};

/*! \brief type of file */
enum FileType {
  /*! \brief the file is file */
  kFile,
  /*! \brief the file is directory */
  kDirectory
};

/*! \brief use to store file information */
struct FileInfo {
  /*! \brief full path to the file */
  URI path;
  /*! \brief the size of the file */
  size_t size;
  /*! \brief the type of the file */
  FileType type;
  /*! \brief default constructor */
  FileInfo() : size(0), type(kFile) {}
};


}  // namespace io
}  // namespace dmlc
#endif  // DMLC_IO_FILESYS_H_
