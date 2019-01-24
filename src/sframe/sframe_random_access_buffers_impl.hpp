/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_RANDOM_ACCESS_BUFFERS_H_
#define TURI_SFRAME_RANDOM_ACCESS_BUFFERS_H_

#include <platform/parallel/lambda_omp.hpp>
#include <util/basic_types.hpp>
#include <util/cityhash_tc.hpp>
#include <util/md5.hpp>
#include <util/string_util.hpp>

using std::enable_shared_from_this;
using std::ios_base;
using std::istream;
using std::make_shared;
using std::map;
using std::pair;
using std::shared_ptr;
using std::streampos;
using std::streamsize;

namespace turi { namespace sframe_random_access {

/**
 * \internal
 * \ingroup sframe_physical
 * \addtogroup sframe_random_access SFrame Random Access Backend
 * \{
 */

inline void write_string_raw(ostream& os, const string& s) {
  os.write(&s[0], s.length());
}

inline string read_string_raw(istream& is, int64_t len) {
  vector<char> buf(len);
  is.read(buf.data(), len);
  return string(buf.data(), len);
}

inline void read_buffer_check(istream& is, const void* data_check, int64_t len) {
  vector<char> buf(len);
  is.read(buf.data(), len);
  bool ok = !memcmp(buf.data(), data_check, len);
  if (!ok) {
    cerr << "read_buffer_check:" << endl;
    cerr << "Expected: " << format_hex(
      string(reinterpret_cast<const char*>(data_check), len)) << endl;
    cerr << "Received: " << format_hex(string(&buf[0], len)) << endl;
    AU();
  }
}

inline void read_string_raw_check(istream& is, const string& s) {
  read_buffer_check(is, &s[0], s.length());
}

template<typename T> struct type_specializer { };

template<typename T> void write_bin(ostream& os, T x);

template<typename T> void write_bin(ostream& os, T x) {
  write_bin(os, x, type_specializer<T>());
}

template<typename T> T read_bin(istream& is);

template<typename T> T read_bin(istream& is) {
  return read_bin(is, type_specializer<T>());
}

template<typename T> void write_bin_pod(ostream& os, T x) {
  os.write(reinterpret_cast<char*>(&x), sizeof(T));
}

template<typename T> T read_bin_pod(istream& is) {
  T ret;
  is.read(reinterpret_cast<char*>(&ret), sizeof(T));
  return ret;
}

#define SERIALIZE_POD(Type) \
  inline void write_bin(ostream& os, Type t, type_specializer<Type>) { \
    write_bin_pod(os, t); \
  } \
  inline Type read_bin(istream& is, type_specializer<Type>) { \
    return read_bin_pod<Type>(is); \
  } \

#define SERIALIZE_DECL(Type) \
  void write_bin(ostream& os, Type t, type_specializer<Type>); \
  Type read_bin(istream& is, type_specializer<Type>);

SERIALIZE_POD(int8_t);
SERIALIZE_POD(char);
SERIALIZE_POD(bool);
SERIALIZE_POD(int32_t);
SERIALIZE_POD(int64_t);

struct object_ids_builtin {
  static const char* pair_;
  static const char* vector_;
};

const uint32_t SFR_VERSION = 0xff010000U; // format version 1 (development)

template<class T> void write_object_header(ostream& os, T*) {
  write_string_raw(os, "SF");
  os.write(reinterpret_cast<const char*>(&SFR_VERSION), sizeof(SFR_VERSION));
  os.write(T::object_id_, strlen(T::object_id_));
}

template<class T> void read_object_header_check(istream& is) {
  read_string_raw_check(is, "SF");
  read_buffer_check(
    is, reinterpret_cast<const char*>(&SFR_VERSION), sizeof(SFR_VERSION));
  read_buffer_check(is, T::object_id_, strlen(T::object_id_));
}

template<typename T> void write_bin(
  ostream& os, optional<T> x, type_specializer<optional<T>>) {

  if (!!x) {
    write_bin(os, int8_t(1));
    write_bin(os, *x);
  } else {
    write_bin(os, int8_t(0));
  }
}

template<typename T> optional<T> read_bin(
  istream& is, type_specializer<optional<T>>) {

  auto b = read_bin<int8_t>(is);
  if (b == 0) {
    return NONE<T>();
  } else {
    auto ret = read_bin<T>(is);
    return SOME<T>(ret);
  }
}

template<typename T, typename U> void write_bin(
  ostream& os, pair<T, U> x, type_specializer<pair<T, U>>) {

  write_bin(os, x.first);
  write_bin(os, x.second);
}

template<typename T, typename U> pair<T, U> read_bin(
  istream& is, type_specializer<pair<T, U>>) {

  auto t = read_bin<T>(is);
  auto u = read_bin<U>(is);
  return make_pair(t, u);
}

template<typename T> void write_bin(
  ostream& os, vector<T> x, type_specializer<vector<T>>) {
  write_bin<int64_t>(os, int64_t(x.size()));
  for (int64_t i = 0; i < len(x); i++) {
    write_bin<T>(os, x[i]);
  }
}

template<typename T> vector<T> read_bin(
  istream& is, type_specializer<vector<T>>) {

  auto n = read_bin<int64_t>(is);
  vector<T> ret;
  for (int64_t i = 0; i < n; i++) {
    ret.push_back(read_bin<T>(is));
  }
  return ret;
}

template<typename T, typename U> void write_bin(
  ostream& os, map<T, U> x, type_specializer<map<T, U>>) {

  write_bin<int64_t>(os, int64_t(x.size()));
  for (auto p : x) {
    write_bin<T>(os, p.first);
    write_bin<U>(os, p.second);
  }
}

template<typename T, typename U> map<T, U> read_bin(
  istream& is, type_specializer<map<T, U>>) {

  auto n = read_bin<int64_t>(is);
  map<T, U> ret;
  for (int64_t i = 0; i < n; i++) {
    auto key = read_bin<T>(is);
    ret[key] = read_bin<U>(is);
  }
  return ret;
}

inline void write_bin(
  ostream& os, string x, type_specializer<string>) {

  write_bin(os, int64_t(x.length()));
  os.write(&x[0], x.length());
}

inline string read_bin(istream& is, type_specializer<string>) {
  auto n = read_bin<int64_t>(is);
  vector<char> ret(n);
  is.read(&ret[0], n);
  return string(&ret[0], n);
}

enum class dtype_enum {
  BOOL,
  I8,
  I16,
  I32,
  I64,
  U8,
  U16,
  U32,
  U64,
  F16,
  F32,
  F64,
};

SERIALIZE_POD(dtype_enum);

inline string dtype_to_string(dtype_enum dtype) {
  switch (dtype) {
  case dtype_enum::BOOL:  return "bool";
  case dtype_enum::I8:    return "int8_t";
  case dtype_enum::U8:    return "uint8_t";
  case dtype_enum::I64:   return "int64_t";
  case dtype_enum::F64:   return "double";
  default:
    cerr << static_cast<int64_t>(dtype) << endl;
    AU();
    return 0;
  }
}

inline ostream& operator<<(ostream& os, dtype_enum dtype) {
  os << dtype_to_string(dtype);
  return os;
}

inline int64_t dtype_size_bytes(dtype_enum dtype) {
  switch (dtype) {
  case dtype_enum::BOOL:  return 1;
  case dtype_enum::I8:    return 1;
  case dtype_enum::U8:    return 1;
  case dtype_enum::I64:   return 8;
  case dtype_enum::F64:   return 8;
  default:
    cerr << dtype << endl;
    AU();
    return 0;
  }
}

inline char dtype_to_char(dtype_enum dtype) {
  switch (dtype) {
  case dtype_enum::BOOL: return 'b';
  case dtype_enum::I8:   return 'c';
  case dtype_enum::U8:   return 'C';
  case dtype_enum::I64:  return 'I';
  case dtype_enum::F64:  return 'd';
  default:
    AU();
    return 0;
  }
}

inline dtype_enum dtype_from_char(char c) {
  switch (c) {
  case 'b': return dtype_enum::BOOL;
  case 'c': return dtype_enum::I8;
  case 'C': return dtype_enum::U8;
  case 'I': return dtype_enum::I64;
  case 'd': return dtype_enum::F64;
  default:
    cerr << "Dtype character not supported: " << c << endl;
    AU();
    return dtype_enum::I8;
  }
}

inline dtype_enum dtype_from_char(const string& s) {
  ASSERT_EQ(s.length(), 1);
  return dtype_from_char(s[0]);
}

inline bool dtype_is_discrete(dtype_enum dtype) {
  switch (dtype) {
  case dtype_enum::BOOL:
  case dtype_enum::I8:
  case dtype_enum::U8:
  case dtype_enum::I16:
  case dtype_enum::U16:
  case dtype_enum::I32:
  case dtype_enum::U32:
  case dtype_enum::I64:
  case dtype_enum::U64:
    return true;
  case dtype_enum::F16:
  case dtype_enum::F32:
  case dtype_enum::F64:
    return false;
  default:
    AU();
    return 0;
  }
}

/**
 * Length of an MD5 hash. Note that we use MD5 as the default for hashing
 * structures in general, but for hashing elements to build column indices, we
 * prefer cityhash due to its significantly faster speed.
 */
constexpr int64_t VALUE_HASH_SIZE_BYTES = 16;

inline string hash_string_value(const char* src, int64_t len) {
  string s(src, len);
  return turi::md5_raw(s);
}

inline string hash_string_value(const string& src) {
  return hash_string_value(src.c_str(), src.length());
}

/**
 * Hashes a given structure recursively (first computing a hash for each of its
 * hashable contents if not already cached).
 */
template<typename T> inline string struct_hash(shared_ptr<T>& x) {
  if (!!x->struct_hash_cached_) {
    string ret = *(x->struct_hash_cached_);
    return ret;
  }

  ostringstream os;
  write_struct_hash_data(os, x);
  string ret = hash_string_value(os.str());
  x->struct_hash_cached_ = SOME(ret);
  return ret;
}

template<typename T> inline string struct_hash(T x) {
  ostringstream os;
  write_struct_hash_data(os, x);
  string ret = hash_string_value(os.str());
  return ret;
}

template<typename T, typename U> inline void write_struct_hash_data(
  ostream& os, pair<T, U> x) {

  write_bin<string>(os, string(object_ids_builtin::pair_));
  write_struct_hash_data(os, x.first);
  write_struct_hash_data(os, x.second);
}

template<typename T> inline void write_struct_hash_data(
  ostream& os, vector<T> x) {

  write_bin<string>(os, string(object_ids_builtin::vector_));
  write_bin<int64_t>(os, x.size());
  for (auto xi : x) {
    write_struct_hash_data(os, xi);
  }
}

inline string format_hex_hash(uint128_t x) {
  auto xs = string(reinterpret_cast<char*>(&x), sizeof(uint128_t));
  return format_hex(xs);
}

/**
 * The number of worker threads, and hence the number of chunks that our 128-bit
 * hash space is divided into (see \ref parallel_hash_map).
 */
inline int64_t get_num_hash_chunks() {
  return turi::thread_pool::get_instance().size();
}

/**
 * The size of a given chunk of the 128-bit hash space (see \ref
 * parallel_hash_map).
 */
inline uint128_t get_hash_chunk_size() {
  thread_local uint128_t ret_cached { 0 };
  thread_local bool ret_is_cached { false };
  if (ret_is_cached) {
    return ret_cached;
  }

  int64_t nt = get_num_hash_chunks();
  uint128_t hash_round = uint128_t(1) << 127;
  uint128_t hash_max = hash_round + (hash_round - 1);
  uint128_t hash_chunk_size = (hash_max / nt) + 1;
  if (nt == 1) {
    hash_chunk_size = hash_max;
  }
  ret_cached = hash_chunk_size;
  ret_is_cached = true;
  return ret_cached;
}

struct bin_handle {
  int64_t index_;
  int64_t offset_;
  int64_t len_;
  bin_handle() : index_(0), offset_(0), len_(0) { }
  bin_handle(int64_t index, int64_t offset, int64_t len)
    : index_(index), offset_(offset), len_(len) { }
};

struct buffer {
  char* addr_;
  int64_t length_;

  buffer(char* addr, int64_t length) : addr_(addr), length_(length) { }
};

DECL_STRUCT(block_manager);

/**
 * Stores, allocates, and reallocates raw binary buffers in memory.
 */
struct block_manager {
  struct block_in_memory {
    char* addr_;
    int64_t length_;
    int64_t capacity_;

    void reserve_length(int64_t new_length);

    block_in_memory() {
      int64_t init_length = 0;
      int64_t init_capacity = (1 << 8);
      length_ = init_length;
      capacity_ = init_capacity;
      addr_ = reinterpret_cast<char*>(malloc(capacity_));
    }

    ~block_in_memory();
  };

  using block_in_memory_p = shared_ptr<block_in_memory>;

  struct handle {
    // TODO: optional (may be paged out to disk/compressed)
    block_in_memory_p block_;

    inline const block_in_memory_p& get_in_memory_view() {
      return block_;
    }

    inline uint128_t get_data_hash(int64_t offset, int64_t len) {
      return turi::hash128(this->get_in_memory_view()->addr_ + offset, len);
    }

    void save_bin_dir(string base_path);

    handle(block_in_memory_p block) : block_(block) { }
  };

  using handle_p = shared_ptr<handle>;

  handle_p load_bin_dir(string base_path);
  handle_p create_block();

  static block_manager* get();
};

bool operator==(
  block_manager::block_in_memory_p, block_manager::block_in_memory_p);
bool operator==(block_manager::handle_p, block_manager::handle_p);

inline buffer binary_data_view_get_data_raw(
  bin_handle h, vector<block_manager::handle_p>& block_handles) {

  return buffer(
    block_handles[h.index_]->get_in_memory_view()->addr_ + h.offset_, h.len_);
}

inline uint128_t binary_data_view_get_data_hash(
  bin_handle h, vector<block_manager::handle_p>& block_handles) {

  return block_handles[h.index_]->get_data_hash(h.offset_, h.len_);
}

inline void binary_data_view_get_data(
  void* dst_addr, bin_handle h,
  vector<block_manager::handle_p>& block_handles) {

  memcpy(
    dst_addr,
    block_handles[h.index_]->get_in_memory_view()->addr_ + h.offset_,
    h.len_);
}

inline void binary_data_view_fixed_get_data(
  void* dst_addr, int64_t offset, int64_t len,
  block_manager::handle_p& block_handle) {

  memcpy(dst_addr, block_handle->block_->addr_ + offset, len);
}

inline string binary_data_view_fixed_get_data_string(
  int64_t offset, int64_t len, block_manager::handle_p block_handle) {

  vector<char> dst(len);
  binary_data_view_fixed_get_data(dst.data(), offset, len, block_handle);
  return string(dst.data(), len);
}

int64_t binary_data_directory_get_file_count(
  const string& base_path, bool is_variable);

string generate_bin_file_path(const string& base_path, int64_t file_index);

/**
 * Provides the basic abstraction of a binary data blob, supporting random
 * access, append operations, and efficient serialization. Note that this
 * version is "fixed"-length, meaning it is backed by a single buffer and does
 * not support multiple concurrent appends.
 */
struct binary_data_builder_fixed {
  int64_t curr_offset_;
  int64_t curr_length_;
  block_manager::handle_p block_handle_;

  inline int64_t get_current_offset() {
    return curr_offset_;
  }

  binary_data_builder_fixed()
    : curr_offset_(0), curr_length_(0),
      block_handle_(block_manager::get()->create_block()) { }

  inline void reserve_length(int64_t new_length) {
    if (new_length <= curr_length_) {
      return;
    }

    auto view = block_handle_->get_in_memory_view();
    view->reserve_length(new_length);
    curr_length_ = new_length;
  }

  inline void put_data_unchecked(int64_t offset, const void* src_addr, int64_t len) {
    memcpy(block_handle_->block_->addr_ + offset, src_addr, len);
  }

  inline void put_data(int64_t offset, const void* src_addr, int64_t len) {
    reserve_length(offset + len);
    put_data_unchecked(offset, src_addr, len);
  }

  inline void get_data_unchecked(void* dst_addr, int64_t offset, int64_t len) {
    memcpy(dst_addr, block_handle_->block_->addr_ + offset, len);
  }

  inline void get_data(void* dst_addr, int64_t offset, int64_t len) {
    ASSERT_LE(offset + len, curr_length_);
    get_data_unchecked(dst_addr, offset, len);
  }

  inline string get_data_string(int64_t offset, int64_t len) {
    vector<char> dst(len);
    get_data(dst.data(), offset, len);
    return string(dst.data(), len);
  }

  inline void append(const void* src_addr, int64_t len) {
    put_data(curr_offset_, src_addr, len);
    curr_offset_ += len;
  }

  inline void append(const string& s) {
    append(&s[0], s.length());
  }

  inline void append_skip(int64_t len) {
    reserve_length(curr_offset_ + len);
    curr_offset_ += len;
  }

  template<typename T> void append_object_header(T*) {
    ostringstream os;
    write_object_header<T>(os, nullptr);
    append(os.str());
  }

  template<typename T> void append_value(const T& val) {
    ostringstream os;
    write_bin(os, val);
    append(os.str());
  }

  inline void save(string base_path) {
    block_handle_->save_bin_dir(base_path);
  }

  binary_data_builder_fixed(const binary_data_builder_fixed&) = delete;
  binary_data_builder_fixed operator=(
    const binary_data_builder_fixed&) = delete;
};

DECL_STRUCT(binary_data_view_fixed);

/**
 * Provides an efficient random-access view on a fully-serialized binary data
 * blob (see \ref binary_data_builder_fixed).
 */
struct binary_data_view_fixed
  : public enable_shared_from_this<binary_data_view_fixed> {

  string base_path_;
  int64_t len_total_;

  block_manager::handle_p block_handle_;

  binary_data_view_fixed(block_manager::handle_p block_handle);
  binary_data_view_fixed(string base_path);

  inline void get_data(void* dst_addr, int64_t offset, int64_t len) {
    binary_data_view_fixed_get_data(
      dst_addr, offset, len, block_handle_);
  }

  string get_data_string(int64_t offset, int64_t len);

  inline void save(string base_path) {
    block_handle_->save_bin_dir(base_path);
  }

  struct istream_reader
    : public boost::iostreams::device<boost::iostreams::input_seekable> {

    binary_data_view_fixed_p src_;
    int64_t curr_offset_;

    istream_reader(binary_data_view_fixed_p src)
      : src_(src), curr_offset_(0) { }

    streamsize read(char* dst, streamsize len);
    streampos seek(boost::iostreams::stream_offset off, ios_base::seekdir way);
  };

  using istream_type = boost::iostreams::stream<istream_reader>;
  using istream_type_p = shared_ptr<istream_type>;

  istream_type_p get_istream();
};

DECL_STRUCT(binary_data_view_variable);

/**
 * Provides an efficient random-access view on a fully-serialized binary data
 * blob (see \ref binary_data_builder_variable).
 */
struct binary_data_view_variable {
  vector<block_manager::handle_p> block_handles_;

  binary_data_view_variable(vector<block_manager::handle_p> block_handles)
    : block_handles_(block_handles) { }
  binary_data_view_variable(string base_path);

  inline void get_data(void* dst_addr, bin_handle h) {
    binary_data_view_get_data(dst_addr, h, block_handles_);
  }

  inline buffer get_data_raw(bin_handle h) {
    return binary_data_view_get_data_raw(h, block_handles_);
  }

  inline uint128_t get_data_hash(bin_handle h) {
    return binary_data_view_get_data_hash(h, block_handles_);
  }

  inline string get_data_string(bin_handle h) {
    vector<char> dst(h.len_);
    get_data(dst.data(), h);
    return string(dst.data(), h.len_);
  }

  inline void save(string base_path) {
    for (int64_t i = 0; i < len(block_handles_); i++) {
      auto dst_path_i = cc_sprintf("%s/%05lld", base_path.c_str(), i);
      at(block_handles_, i)->save_bin_dir(dst_path_i);
    }
  }
};

/**
 * Provides the basic abstraction of a binary data blob, supporting random
 * access, append operations, and efficient serialization. Note that this
 * version is "variable"-length, meaning it is backed by a separate buffer for
 * each worker thread and can support multiple concurrent appends.
 */
struct binary_data_builder_variable {
  int64_t num_workers_max_;
  vector<int64_t> curr_offsets_;
  vector<block_manager::handle_p> block_handles_;

  binary_data_builder_variable(int64_t num_workers_max)
    : num_workers_max_(num_workers_max), curr_offsets_(num_workers_max, 0) {

    for (int64_t i = 0; i < num_workers_max; i++) {
      block_handles_.push_back(block_manager::get()->create_block());
    }
  }

  inline void put_data_unchecked(
    int64_t offset, const void* src_addr, int64_t len, int64_t worker_index) {
    memcpy(
      block_handles_[worker_index]->block_->addr_ + offset, src_addr, len);
  }

  inline void put_data(
    int64_t offset, const void* src_addr, int64_t len, int64_t worker_index) {
    block_handles_[worker_index]->block_->reserve_length(offset + len);
    put_data_unchecked(offset, src_addr, len, worker_index);
  }

  inline bin_handle append(const void* src_addr, int64_t len, int64_t worker_index) {
    auto curr_offset = curr_offsets_[worker_index];
    put_data(curr_offset, src_addr, len, worker_index);
    auto ret = bin_handle(worker_index, curr_offset, len);
    curr_offsets_[worker_index] = curr_offset + len;
    return ret;
  }

  inline void get_data(void* dst_addr, bin_handle h) {
    binary_data_view_get_data(dst_addr, h, block_handles_);
  }

  inline string get_data_string(bin_handle h) {
    vector<char> dst(h.len_);
    get_data(dst.data(), h);
    return string(dst.data(), h.len_);
  }

  binary_data_builder_variable(const binary_data_builder_variable&) = delete;
  binary_data_builder_variable operator=(
    const binary_data_builder_variable&) = delete;
};

}}

#endif
