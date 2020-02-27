/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_S3_FILESYS_HPP_
#define TURI_S3_FILESYS_HPP_

#ifndef TC_DISABLE_REMOTEFS

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/CreateMultipartUploadRequest.h>

#include <core/storage/fileio/s3_api.hpp>
#include <string>
#include <vector>

namespace turi {
namespace fileio {

namespace s3 {

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


struct ScopedAwsInitAPI {
  ScopedAwsInitAPI(Aws::SDKOptions options) : options_(options) {
    Aws::InitAPI(options_);
  }
  ~ScopedAwsInitAPI() { Aws::ShutdownAPI(options_); }
  Aws::SDKOptions options_;
};

class Stream {
 public:
  /*!
   * \brief reads data from a stream
   * \param ptr pointer to a memory buffer
   * \param size block size
   * \return the size of data read
   */
  virtual size_t Read(void *ptr, size_t size) = 0;
  /*!
   * \brief writes data to a stream
   * \param ptr pointer to a memory buffer
   * \param size block size
   */
  virtual void Write(const void *ptr, size_t size) = 0;

  /*!
   * \brief closes the stream
   */
  virtual void Close() = 0;

  /*! \brief virtual destructor */
  virtual ~Stream(void) {}
  /*!
   * \brief generic factory function
   *  create an stream, the stream will close the underlying files upon deletion
   *
   * \param uri the uri of the input currently we support
   *            hdfs://, s3://, and file:// by default file:// will be used
   * \param flag can be "w", "r", "a"
   * \param allow_null whether NULL can be returned, or directly report error
   * \return the created stream, can be NULL when allow_null == true and file do
   * not exist
   */
  static Stream *Create(const char *uri, const char *const flag,
                        bool allow_null = false);
};

/*! \brief interface of i/o stream that support seek */
class SeekStream : public Stream {
 public:
  // virtual destructor
  virtual ~SeekStream(void) {}
  /*! \brief seek to certain position of the file */
  virtual void Seek(size_t pos) = 0;
  /*! \brief tell the position of the stream */
  virtual size_t Tell(void) = 0;
  /*! \brief Returns true if at end of stream */
  virtual bool AtEnd(void) const = 0;
  /*!
   * \brief closes the stream
   */
  virtual void Close() = 0;
  /*!
   * \brief generic factory function
   *  create an SeekStream for read only,
   *  the stream will close the underlying files upon deletion
   *  error will be reported and the system will exit when create failed
   * \param uri the uri of the input currently we support
   *            hdfs://, s3://, and file:// by default file:// will be used
   * \param allow_null whether NULL can be returned, or directly report error
   * \return the created stream, can be NULL when allow_null == true and file do
   * not exist
   */
  static SeekStream *CreateForRead(const char *uri, bool allow_null = false);
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
    std_log_and_throw(std::runtime_error,
                      "AWSReadStreamBase is not supposed to write");
  }
  // lazy seek function
  virtual void Seek(size_t pos) {
    ASSERT_TRUE(pos < file_size_);
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

  void SetBegin(size_t begin_bytes) {
    ASSERT_TRUE(begin_bytes < file_size_);
    curr_bytes_ = begin_bytes;
  }

 protected:
  s3url url_;

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
  ReadStream(const s3url &url, size_t file_size) {
    file_size_ = file_size;
    url_ = url;
  }

  virtual ~ReadStream() {}

 protected:
  // implement InitRequest
  virtual void InitRequest(size_t begin_bytes, s3url &url) {
    SetBegin(begin_bytes);
    url_ = url;
  }
};  // namespace fileio

class WriteStream : public Stream {
 public:
  WriteStream(const s3url &url, bool no_exception = false)
      : url_(url), no_exception_(no_exception) {
    const char *buz = getenv("TURI_S3_WRITE_BUFFER_MB");
    if (buz != nullptr) {
      max_buffer_size_ = static_cast<size_t>(atol(buz)) << 20UL;
    } else {
      // 64 MB
      const size_t kDefaultBufferSize = 64 << 20UL;
      max_buffer_size_ = kDefaultBufferSize;
    }

    InitRequest(url_);
  }

  virtual size_t Read(void *ptr, size_t size) {
    if (no_exception_) {
      std_log_and_throw(std::runtime_error,
                        "S3.WriteStream cannot be used for read");
    }
    return 0;
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
  virtual void InitRequest(s3url &url) {
    InitMultipart(url);
  }

 private:
  // internal maximum buffer size
  size_t max_buffer_size_;
  // path we are reading
  s3url url_;

  bool no_exception_ = false;
  // write data buffer
  std::string buffer_;

  std::string upload_id_;

  // UploadPartOutcomeCallable is fucture<UploadPartOutcome>
  std::vector<Aws::S3::Model::UploadPartOutcomeCallable> completed_parts_;

  bool closed_ = false;

  void InitMultipart(const s3url &url) {
    url_ = url;
    Aws::SDKOptions options;
    ScopedAwsInitAPI aws_init(options);

    auto s3_client = init_aws_sdk_with_turi_env(url_);

    Aws::S3::Model::CreateMultipartUploadRequest create_request;
    create_request.SetBucket(
        Aws::String(url.bucket.c_str(), url.bucket.length()));
    create_request.SetKey(
        Aws::String(url.object_name.c_str(), url.object_name.length()));
    create_request.SetContentType("text/plain");
    auto createMultipartUploadOutcome =
        s3_client.CreateMultipartUpload(create_request);
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
  S3FileSystem(const s3url &url) : url_(url) {}

  virtual ~S3FileSystem() {}

  /*!
   * \brief get information about a path
   * \param path the path to the file
   * \return the information about the file
   */
  virtual FileInfo GetPathInfo(const s3url &path);

  static void ListObjects(const s3url & path, std::vector<FileInfo> &out_list);

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
   * \return the created stream, can be NULL when allow_null == true and file
   * do not exist
   */
  virtual Stream *Open(const s3url &path, const char *const flag);

  /*!
   * \brief open a seekable stream for read
   * \param path the path to the file
   * \return the created stream, can be NULL
   */
  virtual SeekStream *OpenForRead(const s3url &path);

 protected:
  s3url url_;

 private:
  /*!
   * \brief try to get information about a path
   * \param path the path to the file
   * \param out_info holds the path info
   * \return return false when path do not exist
   */
  bool TryGetPathInfo(const s3url &path, FileInfo &info);
};

}  // namespace s3
}  // namespace fileio
}  // namespace turi

#endif

#endif
