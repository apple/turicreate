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

#ifndef TC_DISABLE_REMOTEFS

#include <core/storage/fileio/dmlcio/io.h>

#include <string>
#include <vector>

namespace turi {
namespace fileio {

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
  s3url path;
  /*! \brief the size of the file */
  size_t size;
  /*! \brief the type of the file */
  FileType type;
  /*! \brief default constructor */
  FileInfo() : size(0), type(kFile) {}
};

/*!
 * \brief reader stream that can be used to read from AWS SDK
 */
class AWSReadStreamBase : public SeekStream {
 public:
  virtual ~AWSReadStreamBase() { Close(); }
  virtual void Close() { Reset(file_size_); }

  virtual size_t Tell(void) { return curr_bytes_; }
  virtual bool AtEnd(void) const { return curr_bytes_ == file_size_; }
  virtual void Write(const void *ptr, size_t size) {
    log_and_throw(LOG_FATAL)
        << "AWSReadStreamBase is not supposed to write" << std::endl;
  }
  // lazy seek function
  virtual void Seek(size_t pos) {
    TS_ASSERT(pos < file_size_);
    if (curr_bytes_ != pos) {
      this->Reset(pos);
    }
  }
  virtual size_t Read(void *ptr, size_t size);

 protected:
  AWSReadStreamBase() : file_size_(0), read_ptr_(0), curr_bytes_(0) {}

  /*!
   * \brief initialize the ecurl request,
   * \param begin_bytes the beginning bytes of the stream
   */
  virtual void InitRequest(size_t begin_bytes, s3url &url) = 0;

 protected:
  // the total size of the file
  size_t file_size_ = 0;

  Aws::S3::S3Client s3_client_;

  void SetBegin(size_t begin_bytes) {
    TS_ASSERT_LESS_THAN(begin_bytes, file_size_);
    curr_bytes_ = begin_bytes;
  }

 private:
  /*!
   * \brief called by child class to initialize read
   * \param begin_bytes the beginning bytes of the stream
   */
  void Reset(size_t begin_bytes);

  /*!
   * \brief try to fill the buffer with at least wanted bytes
   * \param want_bytes number of bytes we want to fill
   * \return number of remainning running curl handles
   */

  int FillBuffer(size_t want_bytes);
  // data buffer
  std::string buffer_;
  // data pointer to read position
  size_t read_ptr_;
  // current position in the stream
  size_t curr_bytes_;
};

/*! \brief reader stream that can be used to read */
class ReadStream : public AWSReadStreamBase {
 public:
  ReadStream(const s3url &url, size_t file_size)
      : url_(url), file_size_(file_size) {}

  virtual ~ReadStream() {}

 protected:
  // implement InitRequest
  virtual void InitRequest(size_t begin_bytes, const s3url& url)
    s3_client_ = init_aws_sdk_with_turi_env(url);
    SetBegin(begin_bytes);
  }

 private:
  s3url url_;
};


class WriteStream : public Stream {
 public:
  WriteStream(const s3url &path, bool no_exception = false)
      : path_(path), no_exception_(no_exception) {
    const char *buz = getenv("DMLC_S3_WRITE_BUFFER_MB");
    if (buz != nullptr) {
      max_buffer_size_ = static_cast<size_t>(atol(buz)) << 20UL;
    } else {
      // 64 MB
      const size_t kDefaultBufferSize = 64 << 20UL;
      max_buffer_size_ = kDefaultBufferSize;
    }

    this->InitRequest(path);
  }

  virtual size_t Read(void *ptr, size_t size) {
    if (no_exception_) {
    log_and_throw(LOG_FATAL)
        << "S3.WriteStream cannot be used for read" << std::endl;
  }

  virtual void Write(const void *ptr, size_t size);
  // destructor
  virtual ~WriteStream() {
    if (!closed_) {
      no_exception_ = true;
      Upload(true);
      Finish();
    }
  }

  virtual void Close() {
    closed_ = true;
    Upload(true);
    Finish();
  }

 protected:
  virtual void InitRequest(const s3url &url) {
    s3_client_ = init_aws_sdk_with_turi_env(url);
    SetBegin(begin_bytes);
    InitMultipart(url);
  }

 private:
  // internal maximum buffer size
  size_t max_buffer_size_;
  // path we are reading
  s3url path_;
  // write data buffer
  std::string buffer_;

  std::string upload_id_;

  // UploadPartOutcomeCallable is fucture<UploadPartOutcome>
  std::list<UploadPartOutcomeCallable> completed_parts_;

  Aws::S3::S3Client s3_client_;

  bool closed_ = false;
  bool no_exception_ = false;

  void InitMultipart(const s3url& url) {
    Aws::S3::Model::CreateMultipartUploadRequest createMultipartUploadRequest;
    createMultipartUploadRequest.SetBucket(url.bucket);
    createMultipartUploadRequest.SetKey(url.object_name);
    createMultipartUploadRequest.SetContentType("text/plain");
    auto createMultipartUploadOutcome =
        s3_client_.CreateMultipartUpload(createMultipartUploadRequest);
    ASSERT_TRUE(createMultipartUploadOutcome.IsSuccess());
    upload_id_ = createMultipartUploadOutcome.GetResult().GetUploadId();
  }

  /*!
   * \brief upload the buffer to S3, store the etag
   * clear the buffer
   */
  void Upload(bool force_upload_even_if_zero_bytes = false);

  /*!
   * \brief commit the upload and finish the session
   */
  void Finish(void);
};

class S3FileSystem {
 public:
  /*! \brief constructor */
  S3FileSystem() = default;

  /*! \brief destructor */
  virtual ~S3FileSystem() {}
  /*!
   * \brief get information about a path
   * \param path the path to the file
   * \return the information about the file
   */
  virtual FileInfo GetPathInfo(const s3url &path);
  /*!
   * \brief list files in a directory
   * \param path to the file
   * \param out_list the output information about the files
   */
  virtual void ListDirectory(const s3url &path,
                             std::vector<FileInfo> &out_list);
  /*!
   * \brief open a stream, will report error and exit if bad thing happens
   * NOTE: the Stream can continue to work even when filesystem was destructed
   * \param path path to file
   * \param uri the uri of the input
   * \param flag can be "w", "r", "a"
   * \return the created stream, can be NULL when allow_null == true and file do
   * not exist
   */
  virtual Stream *Open(const s3url &path, const char *const flag);
  /*!
   * \brief open a seekable stream for read
   * \param path the path to the file
   * \return the created stream, can be NULL
   */
  virtual SeekStream *OpenForRead(const s3url &path);
  /*!
   * \brief get a singleton of S3FileSystem when needed
   * \return a singleton instance
   */
  inline static S3FileSystem *GetInstance(void) {
    static S3FileSystem instance;
    return &instance;
  }

 private:
  /*!
   * \brief try to get information about a path
   * \param path the path to the file
   * \param out_info holds the path info
   * \return return false when path do not exist
   */
  bool TryGetPathInfo(const s3url &path, FileInfo &info);

  turi::fileio::s3url url_;
};

}  // namespace fileio
}  // namespace turi

#endif

#endif
