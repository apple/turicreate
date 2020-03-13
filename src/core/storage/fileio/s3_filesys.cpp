#include <aws/core/utils/HashingUtils.h>
#include <aws/s3/model/CompleteMultipartUploadRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/UploadPartRequest.h>

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <core/logging/assertions.hpp>
#include <core/logging/logger.hpp>
#include <core/storage/fileio/get_s3_endpoint.hpp>
#include <core/storage/fileio/s3_api.hpp>
#include <core/storage/fileio/s3_filesys.hpp>
#include <core/storage/fileio/set_curl_options.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>

using turi::fileio::get_bucket_path;
using turi::fileio::set_curl_options;

namespace turi {
namespace fileio {
namespace s3 {

const ScopedAwsInitAPI& turi_global_AWS_SDK_setup(const Aws::SDKOptions &options) {
  static ScopedAwsInitAPI aws_init(options);
  return aws_init;
}

/*!
 * \brief reader stream that can be used to read from AWS SDK
 */
size_t AWSReadStreamBase::Read(void *ptr, size_t size) {
  // check at end
  logstream(LOG_ERROR) << "AWSReadStreamBase::Read: " << curr_bytes_ << std::endl;
  if (curr_bytes_ == file_size_) return 0;

  size_t nleft = size;
  char *buf = reinterpret_cast<char *>(ptr);

  if (read_ptr_ == buffer_.length()) {
    read_ptr_ = 0;
    buffer_.clear();
  } else {
    size_t nread = std::min(nleft, buffer_.length() - read_ptr_);
    std::memcpy(buf, buffer_.data() + read_ptr_, nread);
    buf += nread;
    read_ptr_ += nread;
    nleft -= nread;
    curr_bytes_ += nread;
  }

  // retry is handled by AWS sdk
  if (nleft != 0 && FillBuffer(nleft) != 0) {
    ASSERT_TRUE(buffer_.length() == nleft);
    ASSERT_TRUE(read_ptr_ == 0);
    // nleft is nread
    std::memcpy(buf, buffer_.data() + read_ptr_, nleft);
    read_ptr_ += nleft;
    curr_bytes_ += nleft;
    // nothing left to read
    nleft = 0;
  }

  return size - nleft;
}

// used for restart
void AWSReadStreamBase::Reset(size_t begin_bytes) {
  logstream(LOG_ERROR) << begin_bytes << std::endl;
  // setup the variables
  curr_bytes_ = begin_bytes;
  read_ptr_ = 0;
  buffer_.clear();
}

/*
 * Retry will be handled by AWS SDK.
 *
 * If request is successful, return all bytes read or 0 if no bytes left from
 * s3. If failed, throw turi::io::io_error.
 *
 * @param nwant: number of bytes to read.
 * @return: number of bytes
 */
int AWSReadStreamBase::FillBuffer(size_t nwant) {
  logstream(LOG_INFO) << "FillBuffer: " << nwant << " bytes" << std::endl;
  auto s3_client = init_aws_sdk_with_turi_env(url_);
  // start the new buffer
  read_ptr_ = 0;
  nwant = std::min(nwant, file_size_ - curr_bytes_);
  // nothing to read from remote
  if (nwant == 0) return 0;

  // Get the object
  std::stringstream ss("bytes=");
  // range is includsive and zero based
  ss << curr_bytes_ << '-' << curr_bytes_ + nwant - 1;
  Aws::S3::Model::GetObjectRequest object_request;
  object_request.SetRange(ss.str().c_str());
  object_request.SetBucket(url_.bucket.c_str());
  object_request.SetKey(url_.object_name.c_str());

  auto get_object_outcome = s3_client.GetObject(object_request);
  if (get_object_outcome.IsSuccess()) {
    // Get an Aws::IOStream reference to the retrieved file
    auto &retrieved_file = get_object_outcome.GetResult().GetBody();

    // Output the first line of the retrieved text file
    buffer_.resize(nwant);
    std::memset(const_cast<char *>(buffer_.data()), 0, nwant);
    buffer_.assign((std::istreambuf_iterator<char>(retrieved_file)),
                   std::istreambuf_iterator<char>());
    ASSERT_TRUE(buffer_.size() == nwant);
  } else {
    auto error = get_object_outcome.GetError();
    ss.str("");
    ss << error.GetExceptionName() << ": " << error.GetMessage() << std::endl;
    logstream(LOG_ERROR) << ss.str() << std::endl;
    log_and_throw_io_failure(ss.str());
  }

  return nwant;
}

void WriteStream::Write(const void *ptr, size_t size) {
  size_t rlen = buffer_.length();
  buffer_.resize(rlen + size);
  std::memcpy(const_cast<char *>(buffer_.data()) + rlen, ptr, size);
  if (buffer_.length() >= max_buffer_size_) Upload();
}

void WriteStream::Upload(bool force_upload_even_if_zero_bytes) {
  if (!force_upload_even_if_zero_bytes && buffer_.empty()) return;

  Aws::S3::Model::UploadPartRequest my_request;
  my_request.SetBucket(url_.bucket.c_str());
  my_request.SetKey(url_.object_name.c_str());
  // OMG, the number must start from 1 !!!!
  my_request.SetPartNumber(completed_parts_.size() + 1);
  my_request.SetUploadId(upload_id_.c_str());

  Aws::StringStream ss;
  ss << buffer_.c_str();

  std::shared_ptr<Aws::StringStream> stream_ptr =
      Aws::MakeShared<Aws::StringStream>("WriteStream::Upload", ss.str());

  my_request.SetBody(stream_ptr);

  Aws::Utils::ByteBuffer part_md5(
      Aws::Utils::HashingUtils::CalculateMD5(*stream_ptr));
  my_request.SetContentMD5(Aws::Utils::HashingUtils::Base64Encode(part_md5));

  // it's better to restore the stream cursor
  auto start_pos = stream_ptr->tellg();
  stream_ptr->seekg(0LL, stream_ptr->end);
  my_request.SetContentLength(static_cast<long>(stream_ptr->tellg()));
  stream_ptr->seekg(start_pos);

  // don't forget reset buffer
  buffer_.clear();

  // store the future into completed parts
  completed_parts_.push_back(s3_client_.UploadPartCallable(my_request));
}

void WriteStream::Finish() {
  Aws::S3::Model::CompleteMultipartUploadRequest completedMultipartUploadRequest;
  completedMultipartUploadRequest.SetBucket(url_.bucket.c_str());
  completedMultipartUploadRequest.SetKey(url_.object_name.c_str());
  completedMultipartUploadRequest.SetUploadId(upload_id_.c_str());

  Aws::S3::Model::CompletedMultipartUpload completedMultipartUpload;

  for (size_t ii = 0; ii < completed_parts_.size(); ii++) {
    Aws::S3::Model::UploadPartOutcome my_outcome = completed_parts_[ii].get();
    Aws::S3::Model::CompletedPart completedPart;
    auto etag = my_outcome.GetResult().GetETag();
    ASSERT_TRUE(etag.size());
    completedPart.SetETag(etag);
    completedPart.SetPartNumber(ii + 1);
    completedMultipartUpload.AddParts(completedPart);
  }

  completedMultipartUploadRequest.WithMultipartUpload(completedMultipartUpload);

  auto completeMultipartUploadOutcome =
      s3_client_.CompleteMultipartUpload(completedMultipartUploadRequest);

  if (!completeMultipartUploadOutcome.IsSuccess()) {
    auto error = completeMultipartUploadOutcome.GetError();
    std::stringstream ss;
    ss << error << error.GetExceptionName() << ": " << error.GetMessage() << std::endl;
    logstream(LOG_ERROR) << ss.str() << std::endl;
    log_and_throw_io_failure(ss.str());
  }
}

/*!
 * \brief list the objects in the bucket with prefix specified by path.name
 * \param path the path to query
 * \paam out_list stores the output results
 */
void S3FileSystem::ListObjects(const s3url &path,
                               std::vector<FileInfo> &out_list) {
  logstream(LOG_DEBUG) << "ListObjects:"
                       << "path=" << path.string_from_s3url() << std::endl;

  if (path.bucket.length() == 0) {
    log_and_throw_io_failure("bucket name not specified in S3 URL");
  }
  out_list.clear();
  // use new implementation from s3_api.hpp
  auto ret = list_objects(path.string_from_s3url());
  if (!ret.error.empty()) {
    log_and_throw_io_failure(ret.error);
  }

  // used to decode object name from urls
  std::string err;

  for (size_t ii = 0; ii < ret.objects.size(); ++ii) {
    FileInfo info;
    ASSERT_MSG(ret.objects_size[ii] >= 0, "s3 object size is less than 0");
    info.size = ret.objects_size[ii];
    // reserve metadata except for object_name
    info.path = path;
    s3url tmp;
    parse_s3url(ret.objects[ii], tmp, err);
    if (!err.empty()) {
      log_and_throw_io_failure(err);
    }
    info.path.object_name = tmp.object_name;
    info.type = kFile;
    out_list.push_back(info);
  }

  for (auto &s3dir : ret.directories) {
    FileInfo info;
    info.path = path;
    // add root path to be consistent with other filesys convention
    s3url tmp;
    parse_s3url(s3dir, tmp, err);
    if (!err.empty()) {
      log_and_throw_io_failure(err);
    }
    if (tmp.object_name.size() && tmp.object_name.back() != '/')
      tmp.object_name.append(1, '/');
    info.path.object_name = tmp.object_name;
    info.size = 0;
    info.type = kDirectory;
    out_list.push_back(info);
  }

}  // ListObjects

bool S3FileSystem::TryGetPathInfo(const s3url &url, FileInfo &out_info) {
  // listobjects returns encoded url
  logstream(LOG_DEBUG) << "S3FileSystem::TryGetPathInfo: " << url << std::endl;
  std::string object_name = url.object_name;
  while (object_name.length() > 1 && object_name.back() == '/') {
    object_name.pop_back();
  }
  std::string pdir = object_name + '/';

  std::vector<FileInfo> files;
  ListObjects(url, files);

  for (size_t ii = 0; ii < files.size(); ++ii) {
    if (files[ii].path.object_name == object_name) {
      out_info = files[ii];
      return true;
    }
    if (files[ii].path.object_name == pdir) {
      out_info = files[ii];
      return true;
    }
  }

  return false;
}

FileInfo S3FileSystem::GetPathInfo(const s3url &path) {
  FileInfo info;
  ASSERT_TRUE(TryGetPathInfo(path, info));
  return info;
}

void S3FileSystem::ListDirectory(const s3url &url,
                                 std::vector<FileInfo> &out_list) {
  if (url.object_name.length() && url.object_name.back() == '/') {
    ListObjects(url, out_list);
    return;
  }

  std::vector<FileInfo> files;
  std::string pdir = url.object_name + '/';
  out_list.clear();
  ListObjects(url, files);

  for (size_t i = 0; i < files.size(); ++i) {
    if (files[i].path.object_name == url.object_name) {
      ASSERT_TRUE(files[i].type == kFile);
      out_list.push_back(std::move(files[i]));
      return;
    }

    if (files[i].path.object_name == pdir) {
      ASSERT_TRUE(files[i].type == kDirectory);
      ListObjects(files[i].path, out_list);
      return;
    }
  }
}

Stream *S3FileSystem::Open(const s3url &path, const char *const flag) {
  using namespace std;
  if (!strcmp(flag, "r") || !strcmp(flag, "rb")) {
    return OpenForRead(path);
  } else if (!strcmp(flag, "w") || !strcmp(flag, "wb")) {
    return new s3::WriteStream(path);
  } else {
    log_and_throw_io_failure(
        std::string("S3FileSytem.Open do not support flag ") + flag);
    return nullptr;
  }
}

SeekStream *S3FileSystem::OpenForRead(const s3url &path) {
  FileInfo info;
  if (TryGetPathInfo(path, info) && info.type == kFile) {
    return new s3::ReadStream(path, info.size);
  } else {
    logstream(LOG_WARNING) << "path " << path
                           << " does not exist or is not a file" << std::endl;
    return NULL;
  }
}

}  // namespace s3
}  // namespace fileio
}  // namespace turi