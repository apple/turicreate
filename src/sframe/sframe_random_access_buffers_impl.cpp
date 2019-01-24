/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <sframe/sframe_random_access_buffers_impl.hpp>

#include <sframe/sframe_random_access_impl.hpp>
#include <util/fs_util.hpp>

using turi::fs_util::list_directory;
using turi::fs_util::make_directories_strict;

using std::ifstream;
using std::max;
using std::min;
using std::ofstream;
using std::set;

namespace turi { namespace sframe_random_access {

/**
 * \internal
 * \ingroup sframe_physical
 * \addtogroup sframe_random_access SFrame Random Access Backend
 * \{
 */

block_manager* block_manager::get() {
  static block_manager_p block_manager_singleton =
    make_shared<block_manager>();
  return block_manager_singleton.get();
}

block_manager::handle_p block_manager::create_block() {
  return make_shared<block_manager::handle>(make_shared<block_in_memory>());
}

void block_manager::block_in_memory::reserve_length(int64_t new_length) {
  if (new_length <= capacity_) {
    length_ = new_length;
    return;
  }

  int64_t new_capacity_round = capacity_;
  while (new_capacity_round < new_length) {
    new_capacity_round *= 2;
  }

  addr_ = reinterpret_cast<char*>(realloc(addr_, new_capacity_round));
  ASSERT_TRUE(!!addr_);
  length_ = new_length;
  capacity_ = new_capacity_round;
}

block_manager::block_in_memory::~block_in_memory() {
  free(addr_);
}

void block_manager::handle::save_bin_dir(string base_path) {
  make_directories_strict(base_path);
  auto view = this->get_in_memory_view();
  ofstream os { generate_bin_file_path(base_path, 0) };
  if (view->length_ > 0) {
    os.write(view->addr_, view->length_);
  }
}

block_manager::handle_p block_manager::load_bin_dir(string base_path) {
  auto num_files = list_directory(base_path).size();
  ASSERT_EQ(num_files, 1);
  auto path = generate_bin_file_path(base_path, 0);
  ifstream fin(path);
  fin.seekg(0, ios_base::end);
  int64_t file_len = fin.tellg();
  fin.seekg(0, ios_base::beg);

  auto ret = block_manager::get()->create_block();
  auto ret_view = ret->get_in_memory_view();
  ret_view->reserve_length(file_len);

  fin.read(ret_view->addr_, file_len);
  fin.close();
  return ret;
}

int64_t binary_data_directory_get_file_count(
  const string& base_path, bool is_variable) {

  auto paths = list_directory(base_path);

  if (!is_variable) {
    ASSERT_EQ(paths.size(), 1);
    return 1;
  }

  set<int64_t> bin_indices;

  for (auto path : paths) {
    auto path_list = list_directory(
      cc_sprintf("%s/%s", base_path.c_str(), path.c_str()));
    if (path_list.size() == 0) {
      continue;
    } else if (path_list.size() == 1) {
      auto path_sub = cc_sprintf(
        "%s/%s/00000.bin", base_path.c_str(), path.c_str());
      auto bin_index = string_to_int_check<int64_t>(path);
      bin_indices.insert(bin_index);
    } else {
      // TODO: allow multiple entries to avoid large bin files
      AU();
    }
  }

  int64_t num_files = len(bin_indices);
  if (num_files != 0) {
    ASSERT_EQ(*bin_indices.rbegin() + 1, num_files);
  }

  return num_files;
}

string generate_bin_file_path(const string& base_path, int64_t file_index) {
  return base_path + "/" + cc_sprintf("%05d.bin", file_index);
}

binary_data_view_fixed::binary_data_view_fixed(string base_path)
  : len_total_(0) {

  auto num_files = binary_data_directory_get_file_count(base_path, false);
  ASSERT_EQ(num_files, 1);

  block_handle_ =
    block_manager::get()->load_bin_dir(base_path);

  len_total_ = block_handle_->get_in_memory_view()->length_;
}

binary_data_view_fixed::binary_data_view_fixed(
  block_manager::handle_p block_handle)
  : len_total_(0), block_handle_(block_handle) {

  len_total_ = block_handle_->get_in_memory_view()->length_;
}

string binary_data_view_fixed::get_data_string(int64_t offset, int64_t len) {
  return binary_data_view_fixed_get_data_string(
    offset, len, block_handle_);
}

streamsize binary_data_view_fixed::istream_reader::read(
  char* dst, streamsize len) {

  if (curr_offset_ == src_->len_total_) {
    return -1;
  }
  len = min<int64_t>(len, src_->len_total_ - curr_offset_);
  binary_data_view_fixed_get_data(
    dst, curr_offset_, len, src_->block_handle_);
  curr_offset_ += len;
  return len;
}

streampos binary_data_view_fixed::istream_reader::seek(
  boost::iostreams::stream_offset off, ios_base::seekdir way) {

  if (way == ios_base::cur) {
    curr_offset_ += off;
  } else if (way == ios_base::beg) {
    curr_offset_ = off;
  } else if (way == ios_base::end) {
    curr_offset_ = src_->len_total_ - off;
  } else {
    AU();
  }
  curr_offset_ = max<int64_t>(0, min<int64_t>(src_->len_total_, curr_offset_));
  return curr_offset_;
}

binary_data_view_fixed::istream_type_p binary_data_view_fixed::get_istream() {
  return make_shared<binary_data_view_fixed::istream_type>(shared_from_this());
}

binary_data_view_variable::binary_data_view_variable(string base_path) {
  auto num_files = binary_data_directory_get_file_count(base_path, true);
  for (int64_t i = 0; i < num_files; i++) {
    block_handles_.push_back(
      block_manager::get()->load_bin_dir(
        cc_sprintf("%s/%05d", base_path.c_str(), i)));
  }
}

}}
