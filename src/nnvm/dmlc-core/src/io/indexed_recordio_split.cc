// Copyright by Contributors
#include <dmlc/recordio.h>
#include <dmlc/logging.h>
#include <dmlc/io.h>
#include <algorithm>
#include <fstream>
#include "./indexed_recordio_split.h"

namespace dmlc {
namespace io {

void IndexedRecordIOSplitter::ResetPartition(unsigned rank, unsigned nsplit) {
  size_t ntotal = index_.size();
  size_t ntotalbytes = file_offset_.back();
  size_t nstep = (ntotal + nsplit - 1) / nsplit;
  if (rank * nstep >= ntotal) return;
  index_begin_ = rank * nstep;
  offset_begin_ = index_[index_begin_].first;
  if ((rank + 1) * nstep < ntotal) {
    index_end_ = (rank + 1) * nstep;
    offset_end_ = index_[index_end_].first;
  } else {
    offset_end_ = ntotalbytes;
    index_end_ = index_.size();
    index_.push_back(std::make_pair(offset_end_, 0));
  }
  offset_curr_ = offset_begin_;
  file_ptr_ = std::upper_bound(file_offset_.begin(),
                               file_offset_.end(),
                               offset_begin_) - file_offset_.begin() - 1;
  file_ptr_end_ = std::upper_bound(file_offset_.begin(),
                                   file_offset_.end(),
                                   offset_end_) - file_offset_.begin() - 1;
  if (fs_ != NULL) {
    delete fs_; fs_ = NULL;
  }
  fs_ = filesys_->OpenForRead(files_[file_ptr_].path);
  current_index_ = index_begin_;
  n_overflow_ = 0;
  this->BeforeFirst();
}

void IndexedRecordIOSplitter::ReadIndexFile(FileSystem *fs, const std::string& index_uri) {
  std::vector<URI> expanded_list = this->ConvertToURIs(index_uri);
  CHECK_EQ(expanded_list.size(), 1ul)
    << "IndexedRecordIOSplitter does not support multiple index files";
  for (size_t i = 0; i < expanded_list.size(); ++i) {
    const URI& path = expanded_list[i];
    std::ifstream index_file(path.str());
    std::vector<size_t> temp;
    size_t index, offset;
    while (index_file >> index >> offset) {
      temp.push_back(offset);
    }
    std::sort(temp.begin(), temp.end());
    for (size_t j = 0; j < temp.size() - 1; ++j) {
      index_.push_back(std::make_pair(temp[j], temp[j + 1] - temp[j]));
    }
    index_.push_back(std::make_pair(temp.back(), file_offset_.back() - temp.back()));
  }
}

// Inefficient, but not used anywhere and optimization
// would require change of the API, so I leave it as is
size_t IndexedRecordIOSplitter::SeekRecordBegin(Stream *fi) {
  size_t nstep = 0;
  uint32_t v, lrec;
  while (true) {
    if (fi->Read(&v, sizeof(v)) == 0) return nstep;
    nstep += sizeof(v);
    if (v == RecordIOWriter::kMagic) {
      CHECK(fi->Read(&lrec, sizeof(lrec)) != 0)
            << "invalid record io format";
      nstep += sizeof(lrec);
      uint32_t cflag = RecordIOWriter::DecodeFlag(lrec);
      if (cflag == 0 || cflag == 1) break;
    }
  }
  // should point at head of record
  return nstep - 2 * sizeof(uint32_t);
}

// Inefficient, but not used anywhere and optimization
// would require change of the API, so I leave it as is
const char* IndexedRecordIOSplitter::FindLastRecordBegin(const char *begin,
                                                  const char *end) {
  CHECK_EQ((reinterpret_cast<size_t>(begin) & 3UL), 0U);
  CHECK_EQ((reinterpret_cast<size_t>(end) & 3UL), 0U);
  const uint32_t *pbegin = reinterpret_cast<const uint32_t *>(begin);
  const uint32_t *p = reinterpret_cast<const uint32_t *>(end);
  CHECK(p >= pbegin + 2);
  for (p = p - 2; p != pbegin; --p) {
    if (p[0] == RecordIOWriter::kMagic) {
      uint32_t cflag = RecordIOWriter::DecodeFlag(p[1]);
      if (cflag == 0 || cflag == 1) {
        return reinterpret_cast<const char*>(p);
      }
    }
  }
  return begin;
}

bool IndexedRecordIOSplitter::ExtractNextRecord(Blob *out_rec, Chunk *chunk) {
  if (chunk->begin == chunk->end) return false;
  CHECK(chunk->begin + 2 * sizeof(uint32_t) <= chunk->end)
      << "Invalid RecordIO Format";
  CHECK_EQ((reinterpret_cast<size_t>(chunk->begin) & 3UL), 0U);
  CHECK_EQ((reinterpret_cast<size_t>(chunk->end) & 3UL), 0U);
  uint32_t *p = reinterpret_cast<uint32_t *>(chunk->begin);
  uint32_t cflag = RecordIOWriter::DecodeFlag(p[1]);
  uint32_t clen = RecordIOWriter::DecodeLength(p[1]);
  // skip header
  out_rec->dptr = chunk->begin + 2 * sizeof(uint32_t);
  // move pbegin
  chunk->begin += 2 * sizeof(uint32_t) + (((clen + 3U) >> 2U) << 2U);
  CHECK(chunk->begin <= chunk->end) << "Invalid RecordIO Format";
  out_rec->size = clen;
  if (cflag == 0) return true;
  const uint32_t kMagic = RecordIOWriter::kMagic;
  // abnormal path, move data around to make a full part
  CHECK(cflag == 1U) << "Invalid RecordIO Format";
  while (cflag != 3U) {
    CHECK(chunk->begin + 2 * sizeof(uint32_t) <= chunk->end);
    p = reinterpret_cast<uint32_t *>(chunk->begin);
    CHECK(p[0] == RecordIOWriter::kMagic);
    cflag = RecordIOWriter::DecodeFlag(p[1]);
    clen = RecordIOWriter::DecodeLength(p[1]);
    // pad kmagic in between
    std::memcpy(reinterpret_cast<char*>(out_rec->dptr) + out_rec->size,
                &kMagic, sizeof(kMagic));
    out_rec->size += sizeof(kMagic);
    // move the rest of the blobs
    if (clen != 0) {
      std::memmove(reinterpret_cast<char*>(out_rec->dptr) + out_rec->size,
                   chunk->begin + 2 * sizeof(uint32_t), clen);
      out_rec->size += clen;
    }
    chunk->begin += 2 * sizeof(uint32_t) + (((clen + 3U) >> 2U) << 2U);
  }
  return true;
}

bool IndexedRecordIOSplitter::ReadChunk(void *buf, size_t *size) {
  size_t max_size = *size;
  size_t nread = this->Read(reinterpret_cast<char*>(buf),
                            max_size);
  if (nread == 0) return false;
  if (nread != max_size) {
    *size = nread;
  }
  return true;
}

bool IndexedRecordIOSplitter::NextChunk(Blob *out_chunk) {
  return this->NextBatch(out_chunk, batch_size_);
}

bool IndexedRecordIOSplitter::NextBatchEx(Chunk *chunk, size_t n_records) {
    if (shuffle_) {
      bool ret = true;
      size_t n_read = 0;
      size_t n = n_overflow_ == 0?n_records:n_overflow_;
      while (n_read < n) {
        if (current_index_ < permutation_.size()) {
          offset_curr_ = index_[permutation_[current_index_]].first;
          buffer_size_ = index_[permutation_[current_index_]].second/sizeof(uint32_t);
          size_t new_file_ptr = std::upper_bound(file_offset_.begin(),
                                 file_offset_.end(),
                                 offset_curr_) - file_offset_.begin() - 1;
          if (new_file_ptr != file_ptr_) {
            delete fs_;
            file_ptr_ = new_file_ptr;
            fs_ = filesys_->OpenForRead(files_[file_ptr_].path);
          }
          fs_->Seek(offset_curr_ - file_offset_[file_ptr_]);
          if (n_read == 0) {
            ret = ret && chunk->Load(this, buffer_size_);
          } else {
            ret = ret && chunk->Append(this, buffer_size_);
          }
          if (ret) {
            ++n_read;
            ++current_index_;
          } else {
            break;
          }
        } else {
          break;
        }
      }
      if (n_read > 0) {
        n_overflow_ = n - n_read;
        return true;
      } else {
        return false;
      }
    } else {
      size_t last;
      if (n_overflow_ == 0) {
        last = std::min(current_index_ + n_records, index_end_);
        n_overflow_ = current_index_ + n_records - last;
      } else {
        last = std::min(current_index_ + n_overflow_, index_end_);
        n_overflow_ = current_index_ + n_overflow_ - last;
      }
      buffer_size_ = (index_[last].first - index_[current_index_].first)/INDEXED_RECORDIO_ALIGN;
      current_index_ = last;
      return chunk->Load(this, buffer_size_);
    }
    return true;
}

bool IndexedRecordIOSplitter::NextBatch(Blob *out_chunk, size_t batch_size) {
  while (!ExtractNextChunk(out_chunk, &tmp_chunk_)) {
    if (!NextBatchEx(&tmp_chunk_, batch_size)) return false;
  }
  return true;
}

void IndexedRecordIOSplitter::BeforeFirst(void) {
  if (shuffle_) {
    permutation_.clear();
    for (size_t i = index_begin_; i < index_end_; ++i) {
      permutation_.push_back(i);
    }
    std::shuffle(permutation_.begin(), permutation_.end(), rnd_);
    current_index_ = 0;
  } else {
    current_index_ = index_begin_;
  }
  InputSplitBase::BeforeFirst();
}
}  // namespace io
}  // namespace dmlc
