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
  ScopedAwsInitAPI(const Aws::SDKOptions &options) : options_(options) {
    Aws::InitAPI(options_);
  }
  ~ScopedAwsInitAPI() { Aws::ShutdownAPI(options_); }
  Aws::SDKOptions options_;
};

const ScopedAwsInitAPI &turi_global_AWS_SDK_setup(
    const Aws::SDKOptions &options = Aws::SDKOptions());

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

  /*! tell the physical size of the stream */
  virtual size_t FileSize(void) const = 0;

  /*! \brief Returns true if at end of stream */
  virtual bool AtEnd(void) const = 0;
};

/*!
 * \brief reader stream that should be used to read from AWS SDK
 * there's no buffer in this implementation. Every read will fetch
 * packets through network. So combine this with read_caching_device
 */
class AWSReadStreamBase : public SeekStream {
 public:
  virtual ~AWSReadStreamBase() {
    logstream(LOG_DEBUG) << "~AWSReadStream" << std::endl;
    Close();
  }

  virtual void Close() {
    logstream(LOG_DEBUG) << "AWSReadStream::Close()" << std::endl;
    Reset(file_size_);
  }

  virtual size_t Tell(void) { return curr_bytes_; }

  virtual size_t FileSize() const { return file_size_; }

  virtual bool AtEnd(void) const { return curr_bytes_ == file_size_; }

  virtual void Write(const void *ptr, size_t size) {
    std_log_and_throw(std::runtime_error,
                      "AWSReadStreamBase is not supposed to write");
  }
  // lazy seek function
  virtual void Seek(size_t pos) {
    ASSERT_TRUE(pos < file_size_);
    Reset(pos);
  }

  virtual size_t Read(void *ptr, size_t size);

 protected:
  AWSReadStreamBase() : file_size_(0), curr_bytes_(0) {}

  /*!
   * \brief initialize the ecurl request,
   * \param begin_bytes the beginning bytes of the stream
   */
  virtual void InitRequest(size_t begin_bytes, s3url &url) = 0;

 protected:
  // the total size of the file
  size_t file_size_ = 0;
  s3url url_;

 private:
  /*!
   * \brief called by child class to initialize read
   * \param begin_bytes the beginning bytes of the stream
   */
  void Reset(size_t begin_bytes);

  /*!
   * \brief try to fill the buffer with at least wanted bytes
   * \param buf the output stream pointer provided by client
   * \param want_bytes number of bytes we want to fill
   * \return number of remainning running curl handles
   */

  int FillBuffer(char *buf, size_t want_bytes);
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
    Seek(begin_bytes);
    url_ = url;
  }
};

constexpr size_t S3_MIN_MULTIPART_SIZE = 5 * 1024 * 1024;  // 5MB

class WriteStream : public Stream {
 public:
  WriteStream(const s3url &url, bool no_exception = false)
      : url_(url), no_exception_(no_exception) {
    const char *buz = getenv("TURI_S3_WRITE_BUFFER_MB");
    if (buz != nullptr) {
      max_buffer_size_ = std::max(static_cast<size_t>(atol(buz)) << 20UL,
                                  S3_MIN_MULTIPART_SIZE);
    } else {
      // 64 MB
      const size_t kDefaultBufferSize = 64 << 20UL;
      max_buffer_size_ = kDefaultBufferSize;
    }

    InitRequest(url_);
  }

  virtual size_t Read(void *ptr, size_t size) {
    if (!no_exception_) {
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
    if (!closed_) {
      closed_ = true;
      Upload(true);
      Finish();
    }
  }

 protected:
  virtual void InitRequest(s3url &url) { InitMultipart(url); }

 private:
  // internal maximum buffer size
  size_t max_buffer_size_;
  // path we are reading
  s3url url_;

  bool no_exception_ = false;
  /*
   * write data buffer
   * Aws multipart upload api requries each part to be
   * larger than 5MB, except for the last part.
   **/
  std::string buffer_;

  std::string upload_id_;

  Aws::S3::S3Client s3_client_;

  // UploadPartOutcomeCallable is fucture<UploadPartOutcome>
  std::vector<Aws::S3::Model::UploadPartOutcomeCallable> completed_parts_;

  bool closed_ = false;

  void InitMultipart(const s3url &url) {
    url_ = url;
    s3_client_ = init_aws_sdk_with_turi_env(url_);

    Aws::S3::Model::CreateMultipartUploadRequest create_request;
    create_request.SetBucket(
        Aws::String(url.bucket.c_str(), url.bucket.length()));
    create_request.SetKey(
        Aws::String(url.object_name.c_str(), url.object_name.length()));
    // create_request.SetContentType("text/plain");

    auto createMultipartUploadOutcome =
        s3_client_.CreateMultipartUpload(create_request);

    if (!createMultipartUploadOutcome.IsSuccess()) {
      auto error = createMultipartUploadOutcome.GetError();
      std::stringstream ss;
      ss << error.GetExceptionName() << ": " << error.GetMessage() << std::endl;
      logstream(LOG_ERROR) << ss.str() << std::endl;
      log_and_throw_io_failure(ss.str());
    }

    upload_id_ = createMultipartUploadOutcome.GetResult().GetUploadId().c_str();
  }

  /*!
   * \brief upload the buffer to S3, store the etag
   * clear the buffer.
   *
   * ONLY use this when you finish writing. Aws multipart api
   * only allows the last part to be less than 5MB.
   */
  void Upload(bool force_upload = false);

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

  static void ListObjects(const s3url &path, std::vector<FileInfo> &out_list);

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
  virtual std::shared_ptr<Stream> Open(const s3url &path,
                                       const char *const flag);

  /*!
   * \brief open a seekable stream for read
   * \param path the path to the file
   * \return the created stream, can be NULL if no_exception is true to
   * indicate a failure to open the file.
   */
  virtual std::shared_ptr<SeekStream> OpenForRead(const s3url &path,
                                                  bool no_exception = true);

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
