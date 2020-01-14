#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>

#include <core/storage/fileio/dmlcio/filesys.h>
#include <core/storage/fileio/dmlcio/io.h>
#include <boost/algorithm/string.hpp>
#include <core/logging/assertions.hpp>
#include <core/storage/fileio/get_s3_endpoint.hpp>
#include <core/storage/fileio/set_curl_options.hpp>

extern "C" {
#include <curl/curl.h>
#include <errno.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/hmac.h>
#include <openssl/md5.h>
}

using turi::fileio::get_bucket_path;
using turi::fileio::set_curl_options;

/*!
 * \brief safely get the beginning address of a vector
 * \param vec input vector
 * \return beginning address of a vector
 */
template <typename T>
static inline T *BeginPtr(std::vector<T> &vec) {
  if (vec.size() == 0) {
    return NULL;
  } else {
    return &vec[0];
  }
}
/*! \brief get the beginning address of a vector */
template <typename T>
static inline const T *BeginPtr(const std::vector<T> &vec) {
  if (vec.size() == 0) {
    return NULL;
  } else {
    return &vec[0];
  }
}
static inline char *BeginPtr(std::string &str) {
  if (str.length() == 0) return NULL;
  return &str[0];
}
static inline const char *BeginPtr(const std::string &str) {
  if (str.length() == 0) return NULL;
  return &str[0];
}

namespace dmlc {
namespace io {

// remove the beginning slash at name
inline const char *RemoveBeginSlash(const std::string &name) {
  const char *s = name.c_str();
  while (*s == '/') {
    ++s;
  }
  return s;
}

// fin dthe error field of the header
static inline bool FindHttpError(const std::string &header) {
  std::string hd, ret;
  int code;
  std::istringstream is(header);
  if (is >> hd >> code >> ret) {
    if (code == 206 || ret == "OK") {
      return false;
    } else if (ret == "Continue") {
      return false;
    }
  }
  return true;
}

// curl callback to write sstream
size_t WriteSStreamCallback(char *buf, size_t size, size_t count, void *fp) {
  static_cast<std::ostringstream *>(fp)->write(buf, size * count);
  return size * count;
}
// callback by curl to write to std::string
size_t WriteStringCallback(char *buf, size_t size, size_t count, void *fp) {
  size *= count;
  std::string *str = static_cast<std::string *>(fp);
  size_t len = str->length();
  str->resize(len + size);
  std::memcpy(BeginPtr(*str) + len, buf, size);
  return size;
}

// useful callback for reading memory
struct ReadStringStream {
  const char *dptr;
  size_t nleft;
  // constructor
  ReadStringStream(const std::string &data) {
    dptr = BeginPtr(data);
    nleft = data.length();
  }
  // curl callback to write sstream
  static size_t Callback(char *buf, size_t size, size_t count, void *fp) {
    size *= count;
    ReadStringStream *s = static_cast<ReadStringStream *>(fp);
    size_t nread = std::min(size, s->nleft);
    std::memcpy(buf, s->dptr, nread);
    s->dptr += nread;
    s->nleft -= nread;
    return nread;
  }
};

/*!
 * \brief reader stream that can be used to read from CURL
 */
class CURLReadStreamBase : public SeekStream {
 public:
  virtual ~CURLReadStreamBase() { Close(); }
  virtual void Close() { this->Cleanup(); }

  virtual size_t Tell(void) { return curr_bytes_; }
  virtual bool AtEnd(void) const { return at_end_; }
  virtual void Write(const void *ptr, size_t size) {
    logstream(LOG_FATAL) << "CURL.ReadStream cannot be used for write"
                         << std::endl;
  }
  // lazy seek function
  virtual void Seek(size_t pos) {
    if (curr_bytes_ != pos) {
      this->Cleanup();
      curr_bytes_ = pos;
    }
  }
  virtual size_t Read(void *ptr, size_t size);

 protected:
  CURLReadStreamBase()
      : mcurl_(NULL),
        ecurl_(NULL),
        slist_(NULL),
        read_ptr_(0),
        curr_bytes_(0),
        at_end_(false) {}
  /*!
   * \brief initialize the ecurl request,
   * \param begin_bytes the beginning bytes of the stream
   * \param ecurl a curl easy handle that can be used to set request
   * \param slist a curl slist handle that can be used to set headers
   */
  virtual void InitRequest(size_t begin_bytes, CURL *ecurl,
                           curl_slist **slist) = 0;

 protected:
  // the total size of the file
  size_t expect_file_size_ = 0;

 private:
  /*!
   * \brief called by child class to initialize read
   * \param begin_bytes the beginning bytes of the stream
   */
  void Init(size_t begin_bytes);
  /*!
   * \brief cleanup the previous session for restart
   */
  void Cleanup(void);
  /*!
   * \brief try to fill the buffer with at least wanted bytes
   * \param want_bytes number of bytes we want to fill
   * \return number of remainning running curl handles
   */
  int FillBuffer(size_t want_bytes);
  // multi and easy curl handle
  CURL *mcurl_, *ecurl_;
  // slist needed by the program
  curl_slist *slist_;
  // data buffer
  std::string buffer_;
  // header buffer
  std::string header_;
  // data pointer to read position
  size_t read_ptr_;
  // current position in the stream
  size_t curr_bytes_;
  // mark end of stream
  bool at_end_;
};

// read data in
size_t CURLReadStreamBase::Read(void *ptr, size_t size) {
  // lazy initialize
  if (mcurl_ == NULL) Init(curr_bytes_);
  // check at end
  if (at_end_) return 0;

  size_t nleft = size;
  char *buf = reinterpret_cast<char *>(ptr);

  size_t retry_count = 0;
  while (nleft != 0) {
    while (nleft != 0) {
      if (read_ptr_ == buffer_.length()) {
        read_ptr_ = 0;
        buffer_.clear();
        if (this->FillBuffer(nleft) == 0 && buffer_.length() == 0) {
          at_end_ = true;
          break;
        }
      }
      size_t nread = std::min(nleft, buffer_.length() - read_ptr_);
      std::memcpy(buf, BeginPtr(buffer_) + read_ptr_, nread);
      buf += nread;
      read_ptr_ += nread;
      nleft -= nread;
    }
    size_t read_bytes = size - nleft;
    curr_bytes_ += read_bytes;

    // retry
    if (retry_count < 5 && nleft > 0 && at_end_ && expect_file_size_ != 0 &&
        curr_bytes_ != expect_file_size_) {
      // reestablish connection;
      size_t old_curr_bytes = curr_bytes_;
      Cleanup();
      Init(old_curr_bytes);
      ++retry_count;
    } else {
      break;
    }
  }
  return size - nleft;
}

// cleanup the previous sessions for restart
void CURLReadStreamBase::Cleanup() {
  if (mcurl_ != NULL) {
    curl_multi_remove_handle(mcurl_, ecurl_);
    curl_easy_cleanup(ecurl_);
    curl_multi_cleanup(mcurl_);
    mcurl_ = NULL;
    ecurl_ = NULL;
  }
  if (slist_ != NULL) {
    curl_slist_free_all(slist_);
    slist_ = NULL;
  }
  buffer_.clear();
  header_.clear();
  curr_bytes_ = 0;
  at_end_ = false;
}

void CURLReadStreamBase::Init(size_t begin_bytes) {
  ASSERT_MSG(mcurl_ == NULL && ecurl_ == NULL && slist_ == NULL,
             "must call init in clean state");
  // make request
  ecurl_ = curl_easy_init();
  this->InitRequest(begin_bytes, ecurl_, &slist_);
  ASSERT_TRUE(curl_easy_setopt(ecurl_, CURLOPT_WRITEFUNCTION,
                               WriteStringCallback) == CURLE_OK);
  ASSERT_TRUE(curl_easy_setopt(ecurl_, CURLOPT_WRITEDATA, &buffer_) ==
              CURLE_OK);
  ASSERT_TRUE(curl_easy_setopt(ecurl_, CURLOPT_HEADERFUNCTION,
                               WriteStringCallback) == CURLE_OK);
  ASSERT_TRUE(curl_easy_setopt(ecurl_, CURLOPT_HEADERDATA, &header_) ==
              CURLE_OK);
  set_curl_options(ecurl_);
  curl_easy_setopt(ecurl_, CURLOPT_NOSIGNAL, 1);
  mcurl_ = curl_multi_init();
  ASSERT_TRUE(curl_multi_add_handle(mcurl_, ecurl_) == CURLM_OK);
  int nrun;
  curl_multi_perform(mcurl_, &nrun);
  ASSERT_TRUE(nrun != 0 || header_.length() != 0 || buffer_.length() != 0);
  // start running and check header
  this->FillBuffer(1);
  if (FindHttpError(header_)) {
    while (this->FillBuffer(buffer_.length() + 256) != 0)
      ;
    std::string message = std::string("Request Error") + header_ + buffer_;
    log_and_throw_io_failure(message);
  }
  // setup the variables
  at_end_ = false;
  curr_bytes_ = begin_bytes;
  read_ptr_ = 0;
}

// fill the buffer with wanted bytes
int CURLReadStreamBase::FillBuffer(size_t nwant) {
  int nrun = 0;
  while (buffer_.length() < nwant) {
    // wait for the event of read ready
    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);
    int maxfd = -1;
    // Curl default timeout as in:
    // https://curl.haxx.se/libcurl/c/curl_multi_timeout.html
    timeval timeout;
    long curl_timeo = 0;
    curl_multi_timeout(mcurl_, &curl_timeo);
    if (curl_timeo < 0) curl_timeo = 980;
    timeout.tv_sec = curl_timeo / 1000;
    timeout.tv_usec = (curl_timeo % 1000) * 1000;
    ASSERT_TRUE(curl_multi_fdset(mcurl_, &fdread, &fdwrite, &fdexcep, &maxfd) ==
                CURLM_OK);
    int rc;
    if (maxfd == -1) {
#ifdef _WIN32
      Sleep(100);
      rc = 0;
#else
      struct timeval wait = {0, 100 * 1000};
      rc = select(0, NULL, NULL, NULL, &wait);
#endif
    } else {
      rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
    }
    if (rc != -1) {
      CURLMcode ret = curl_multi_perform(mcurl_, &nrun);
      if (ret == CURLM_CALL_MULTI_PERFORM) continue;
      ASSERT_TRUE(ret == CURLM_OK);
      if (nrun == 0) break;
    }
  }
  // loop through all the subtasks in curl_multi_perform and look for errors
  struct CURLMsg *m;
  do {
    int msgq = 0;
    m = curl_multi_info_read(mcurl_, &msgq);
    if (m && (m->msg == CURLMSG_DONE)) {
      if (m->data.result != CURLE_OK) {
        logstream(LOG_ERROR) << curl_easy_strerror(m->data.result) << std::endl;
      }
    }
  } while (m);

  return nrun;
}
// End of CURLReadStreamBase functions

// singleton class for global initialization
struct CURLGlobal {
  CURLGlobal() {
    ASSERT_TRUE(curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK);
  }
  ~CURLGlobal() { curl_global_cleanup(); }
};

// used for global initialization
static CURLGlobal curl_global;

/*! \brief reader stream that can be used to read */
class ReadStream : public CURLReadStreamBase {
 public:
  ReadStream(const URI &path, const std::string &aws_id,
             const std::string &aws_key, size_t file_size)
      : path_(path), aws_id_(aws_id), aws_key_(aws_key) {
    this->expect_file_size_ = file_size;
  }
  virtual ~ReadStream(void) {}

 protected:
  // implement InitRequest
  virtual void InitRequest(size_t begin_bytes, CURL *ecurl, curl_slist **slist);

 private:
  // path we are reading
  URI path_;
  // aws access key and id
  std::string aws_id_, aws_key_, aws_region_;
};

// initialize the reader at begin bytes
void ReadStream::InitRequest(size_t begin_bytes, CURL *ecurl,
                             curl_slist **slist) {
  // initialize the curl request
  std::vector<std::string> amz;
  std::string date = GetDateString();

  auto lowerhost = boost::algorithm::to_lower_copy(path_.host);
  std::string signature =
      Sign(aws_key_, "GET", "", "", date, amz,
           std::string("/") + lowerhost + '/' + RemoveBeginSlash(path_.name));
  // generate headers
  std::ostringstream sauth, sdate, surl, srange;
  std::ostringstream result;
  sauth << "Authorization: AWS " << aws_id_ << ":" << signature;
  sdate << "Date: " << date;
  surl << get_bucket_path(path_.host) << RemoveBeginSlash(path_.name);
  srange << "Range: bytes=" << begin_bytes << "-";
  *slist = curl_slist_append(*slist, sdate.str().c_str());
  *slist = curl_slist_append(*slist, srange.str().c_str());
  *slist = curl_slist_append(*slist, sauth.str().c_str());
  ASSERT_TRUE(curl_easy_setopt(ecurl, CURLOPT_HTTPHEADER, *slist) == CURLE_OK);
  ASSERT_TRUE(curl_easy_setopt(ecurl, CURLOPT_URL, surl.str().c_str()) ==
              CURLE_OK);
  ASSERT_TRUE(curl_easy_setopt(ecurl, CURLOPT_HTTPGET, 1L) == CURLE_OK);
  ASSERT_TRUE(curl_easy_setopt(ecurl, CURLOPT_HEADER, 0L) == CURLE_OK);
  ASSERT_TRUE(curl_easy_setopt(ecurl, CURLOPT_NOSIGNAL, 1) == CURLE_OK);
}

/*! \brief simple http read stream to check */
class HttpReadStream : public CURLReadStreamBase {
 public:
  HttpReadStream(const URI &path) : path_(path) {}
  // implement InitRequest
  virtual void InitRequest(size_t begin_bytes, CURL *ecurl,
                           curl_slist **slist) {
    ASSERT_MSG(begin_bytes == 0, " HttpReadStream: do not support Seek");
    ASSERT_TRUE(curl_easy_setopt(ecurl, CURLOPT_URL, path_.str().c_str()) ==
                CURLE_OK);
    ASSERT_TRUE(curl_easy_setopt(ecurl, CURLOPT_NOSIGNAL, 1) == CURLE_OK);
  }

 private:
  URI path_;
};

inline void InitS3Client(const s3url &parsed_url, std::string& proxy, S3Client& client) {
    // initialization
    Aws::SDKOptions options;
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;
    Aws::InitAPI(options);

    // credentials
    Aws::Auth::AWSCredentials credentials(parsed_url.access_key_id.c_str(), parsed_url.secret_key.c_str());

    // s3 client config
    Aws::Client::ClientConfiguration clientConfiguration;
    if (turi::fileio::insecure_ssl_cert_checks()) {
      clientConfiguration.verifySSL = false;
    }
    if (parsed_url.endpoint.empty()) {
      clientConfiguration.endpointOverride = endpoint.c_str();
    } else {
      clientConfiguration.endpointOverride = parsed_url.endpoint.c_str();
    }

    clientConfiguration.proxyHost = proxy.c_str();
    clientConfiguration.requestTimeoutMs = 5 * 60000;
    clientConfiguration.connectTimeoutMs = 20000;
    std::string region = fileio::get_region_name_from_endpoint(clientConfiguration.endpointOverride.c_str());
    clientConfiguration.region = region.c_str();

    client = std::move(S3Client(credentials, clientConfiguration));
}

class WriteStream : public Stream {
 public:
  WriteStream(const s3url &path) : path_(path) {
    const char *buz = getenv("DMLC_S3_WRITE_BUFFER_MB");
    if (buz != NULL) {
      max_buffer_size_ = static_cast<size_t>(atol(buz)) << 20UL;
    } else {
      // 64 MB
      const size_t kDefaultBufferSize = 64 << 20UL;
      max_buffer_size_ = kDefaultBufferSize;
    }
    max_error_retry_ = 3;
    this->Init();
  }

  virtual size_t Read(void *ptr, size_t size) {
    logstream(LOG_FATAL) << "S3.WriteStream cannot be used for read"
                         << std::endl;
    return 0;
  }
  virtual void Write(const void *ptr, size_t size);
  // destructor
  virtual ~WriteStream() {
    if (!closed_) {
      no_exception_ = true;
      this->Upload(true);
      this->Finish();
      curl_easy_cleanup(ecurl_);
    }
  }

  virtual void Close() {
    closed_ = true;
    this->Upload(true);
    this->Finish();
    curl_easy_cleanup(ecurl_);
  }

 private:
  // internal maximum buffer size
  size_t max_buffer_size_;
  // maximum time of retry when error occurs
  int max_error_retry_;
  // path we are reading
  s3url path_;
  // write data buffer
  std::string buffer_;
  // etags of each part we uploaded
  std::vector<std::string> etags_;
  // part id of each part we uploaded
  std::vector<size_t> part_ids_;

  Aws::S3::S3Client client_;

  bool closed_ = false;

  bool no_exception_ = false;
  /*!
   * \brief helper function to do http post request
   * \param method method to peform
   * \param path the resource to post
   * \param url_args additional arguments in URL
   * \param url_args translated arguments to sign
   * \param content_type content type of the data
   * \param data data to post
   * \param out_header holds output Header
   * \param out_data holds output data
   */
  void Run(const std::string &method, const URI &path, const std::string &args,
           const std::string &content_type, const std::string &data,
           std::string *out_header, std::string *out_data);
  /*!
   * \brief initialize the upload request
   */
  void Init(void);
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

void WriteStream::Init(void) {
  InitS3Client(path_, "", client_)
}

void WriteStream::Write(const void *ptr, size_t size) {
  size_t rlen = buffer_.length();
  buffer_.resize(rlen + size);
  std::memcpy(BeginPtr(buffer_) + rlen, ptr, size);
  if (buffer_.length() >= max_buffer_size_) {
    this->Upload();
  }
}

void WriteStream::Run(const std::string &method, const s3url &path,
                      const std::string &content_type, const std::string &data,
                      std::string &etag) {
  int num_retry = 0;

  const std::shared_ptr<Aws::IOStream> input_data =
      Aws::MakeShared<Aws::FStream>("SampleAllocationTag", file_name.c_str(),
                                    std::ios_base::in | std::ios_base::binary);

  while (num_retry < max_error_retry_) {
    // helper for read string
    if (method == "POST") {
      Aws::S3::Model::PostObjectRequest object_request;

      object_request.SetBucket(path.bucket);
      object_request.SetKey(path.object_name);
      object_request.SetBody(input_data);
      object_request.setContentType(content_type);

      // Put the object
      auto outcome = s3_client.PostObject(object_request);
      if (!outcome.IsSuccess()) {
        auto error = outcome.GetError();
        logstream(LOG_ERROR) << "ERROR: " << error.GetExceptionName() << ": "
                             << error.GetMessage() << std::endl;
        num_retry++;
      } else {
        etag = outcome.GetEtag();
      }

    } else if (method == "PUT") {
      Aws::S3::Model::PutObjectRequest object_request;

      object_request.SetBucket(path.bucket);
      object_request.SetKey(path.object_name);
      object_request.setContentType(content_type);

      object_request.SetBody(input_data);

      // Put the object
      auto outcome = s3client_.PutObject(object_request);
      if (!outcome.IsSuccess()) {
        auto error = outcome.GetError();
        logstream(LOG_ERROR) << "ERROR: " << error.GetExceptionName() << ": "
                             << error.GetMessage() << std::endl;
        num_retry++;
      } else {
        etag = outcome.GetEtag();
      }
    }
  }
}

void WriteStream::Upload(bool force_upload_even_if_zero_bytes) {
  if (buffer_.length() == 0 && !force_upload_even_if_zero_bytes) return;
  std::string etag;
  size_t partno = etags_.size() + 1;
  Run("PUT", path_, sarg.str(), "binary/octel-stream", buffer_, etag);
  etags_.push_back(std::move(etag));
  part_ids_.push_back(partno);
  buffer_.clear();
}

void WriteStream::Finish(void) {
  request.SetUploadId(InitMultipart.GetResult().GetUploadId());
  Aws::S3::Model::CompleteMultipartUploadRequest request;
  Aws::S3::Model::CompletePart part;
  for (size_t ii = 0; ii < etags_.size(); ++ii) {
    part.SetETag(etags_[ii]);
    part.SetNumber(part_ids_[ii]);
    request.AddParts(part)
  }
  auto outcome = s3client_.CompleteMultipartUpload(request);
}

/*!
 * \brief list the objects in the bucket with prefix specified by path.name
 * \param path the path to query
 * \paam out_list stores the output results
 */
void ListObjects(const s3url &path, std::vector<FileInfo> *out_list) {
  if (path.host.length() == 0) {
    log_and_throw_io_failure("bucket name not specified in S3 URI");
  }
  out_list->clear();
  // use new implementation from s3_api.hpp
  auto ret = fileio::list_objects(path);

  for (auto &s3file : ret.objects) {
    FileInfo info;
    ASSERT_MSG(s3file.second >= 0, "s3 object size is less than 0");
    info.size = static_cast<size_t>(s3file.second);
    info.path = path;
    // add root path to be consistent with other filesys convention
    info.path.name = '/' + s3file.first;
    info.type = kFile;
    out_list->push_back(info);
  }

  for (auto &s3dir : ret.directories) {
    FileInfo info;
    info.path = path;
    // add root path to be consistent with other filesys convention
    info.path.name = '/' + s3dir.first;
    info.size = 0;
    info.type = kDirectory;
    out_list->push_back(info);
  }

}  // ListObjects

}  // namespace io

bool S3FileSystem::TryGetPathInfo(const s3url &path_, FileInfo *out_info) {
  URI path = path_;
  while (path.name.length() > 1 && path.name.back() == '/') {
    path.name.resize(path.name.length() - 1);
  }
  std::vector<FileInfo> files;
  s3::ListObjects(path, &files);
  std::string pdir = path.name + '/';
  for (size_t i = 0; i < files.size(); ++i) {
    if (files[i].path.name == path.name) {
      *out_info = files[i];
      return true;
    }
    if (files[i].path.name == pdir) {
      *out_info = files[i];
      return true;
    }
  }
  return false;
}

FileInfo S3FileSystem::GetPathInfo(const s3url &path) {
  FileInfo info;
  ASSERT_TRUE(TryGetPathInfo(path, &info));
  return info;
}

void S3FileSystem::ListDirectory(const s3url &path,
                                 std::vector<FileInfo> *out_list) {

  if (path.name[path.name.length() - 1] == '/') {
    s3::ListObjects(path, out_list);
    return;
  }

  std::vector<FileInfo> files;
  std::string pdir = path.name + '/';
  out_list->clear();
  s3::ListObjects(path, &files);

  for (size_t i = 0; i < files.size(); ++i) {
    if (files[i].path.name == path.name) {
      ASSERT_TRUE(files[i].type == kFile);
      out_list->push_back(files[i]);
      return;
    }
    if (files[i].path.name == pdir) {
      ASSERT_TRUE(files[i].type == kDirectory);
      s3::ListObjects(files[i].path, out_list);
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
    return NULL;
  }
}

SeekStream *S3FileSystem::OpenForRead(const s3url &path) {
  FileInfo info;
  if (TryGetPathInfo(path, &info) && info.type == kFile) {
    return new s3::ReadStream(path, info.size);
  } else {
    return NULL;
  }
}
}  // namespace dmlc
}  // namespace dmlc
