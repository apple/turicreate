/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <boost/algorithm/string.hpp>
#include <future>
#include <minipsutil/minipsutil.h>
#include <unity/toolkits/supervised_learning/xgboost_iterator.hpp>
#include <util/dense_bitset.hpp>

#include <xgboost/src/utils/io.h>
#include <xgboost/src/learner/learner-inl.hpp>
#include <xgboost/src/io/simple_dmatrix-inl.hpp>
#include <xgboost/src/io/simple_fmatrix-inl.hpp>
#include <xgboost/src/io/page_fmatrix-inl.hpp>

namespace turi {
namespace supervised {
namespace xgboost {

using namespace ::xgboost;
using namespace ::xgboost::io;
using namespace ::xgboost::learner;

/**************************************************************************/
/*                                                                        */
/*                        Row Batch Iterator from MLData                  */
/*                                                                        */
/**************************************************************************/
/*! \brief adapts ml_data_iterator to RowBatchIterator */
class MLDataBatchIterator : public utils::IIterator<RowBatch> {
 public:
  MLDataBatchIterator(const ml_data &ml_data,
                      size_t rows_per_batch,
                      size_t nthreads = 1)
      : ml_data_(ml_data), rows_per_batch_(rows_per_batch), nthreads_(nthreads){

    rows_per_batch_ = std::max<size_t>(1, rows_per_batch_);
    num_batches_ = (ml_data.num_rows() + rows_per_batch_ - 1) / rows_per_batch_;

    // Initialize thread buffer
    tl_row_buffer_.resize(nthreads_);
    for (size_t i = 0; i < nthreads_; ++i) {
      tl_iter_.push_back(ml_data_.get_iterator());
    }

    // Zero the output
    memset(&(this->out_), 0, sizeof(this->out_));
  }

  virtual void BeforeFirst(void) {
    current_row_ = 0;
  }

  virtual bool Next(void) {
    if (current_row_ == ml_data_.num_rows()) {
      return false;
    }
    if (num_batches_ == 1 && out_.size > 0) {
      // fast pass
      // skip fetch because we have already fetched everything into out_
      // set current_row to end
      current_row_ = ml_data_.num_rows();
    } else {
      // regular pass
      // clear current data, and fetch new rows
      this->FetchToBuffer();
    }
    return true;
  }
  virtual const RowBatch &Value(void) const {
    return out_;
  }
  size_t num_batches () const {
    return num_batches_;
  }

 private:
  // Store rows in CSR format
  struct RowBlock {
    RowBlock() { Reset(); }
    void Reset() {
      entry_ptr_.resize(1);
      entry_ptr_[0] = 0;
      entries_.clear();
      entries_.shrink_to_fit();
      entry_ptr_.shrink_to_fit();
    }
    size_t Size() { return entry_ptr_.size() - 1; }
    // row pointer of CSR format
    std::vector<size_t> entry_ptr_;
    // data of CSR format
    std::vector<RowBatch::Entry> entries_;
  };

 private:
  void FetchSingleRow(ml_data_iterator& iter, RowBlock& row_block) {
    iter->unpack(
      [&](ml_column_mode mode, size_t column_index,
          size_t feature_index, double value,
          size_t index_size, size_t index_offset) {
        // Treat NAN as missing value
        if(feature_index < index_size && !std::isnan(value)) {
          bst_uint findex = static_cast<bst_uint>(index_offset + feature_index);
          row_block.entries_.push_back(RowBatch::Entry(findex, value));
        }
      },
      [&](ml_column_mode mode, size_t column_index, size_t index_size) {});
    row_block.entry_ptr_.push_back(row_block.entries_.size());
  }
  void FetchToBuffer() {
    this->ResetBuffer();
    {
      // reserve spaces for thread local buffer
      for (auto& buffer: tl_row_buffer_) {
        buffer.entry_ptr_.reserve(rows_per_batch_ / nthreads_);
        buffer.entries_.reserve(rows_per_batch_ * ml_data_.max_row_size() / nthreads_);
      }
    }
    size_t begin_row = current_row_;
    size_t end_row = std::min(current_row_ + rows_per_batch_, ml_data_.num_rows());

    // In parallel fetch data into thread local buffers
    parallel_for (0, nthreads_, [&](size_t thread_id) {
      size_t total_to_fetch = (end_row - begin_row);
      size_t tl_begin_row = (begin_row) + (total_to_fetch) * thread_id / nthreads_;
      size_t tl_end_row = (begin_row) + (total_to_fetch) * (thread_id + 1) / nthreads_;
      auto& iter = tl_iter_[thread_id];
      iter.seek(tl_begin_row);
      RowBlock& buffer = tl_row_buffer_[thread_id];
      while (tl_begin_row < tl_end_row) {
        this->FetchSingleRow(iter, buffer);
        ++iter;
        ++tl_begin_row;
      }
    });

    size_t total_entries = 0;
    for (auto& buffer: tl_row_buffer_) {
      total_entries += buffer.entries_.size();
    }

    // Merge thread local buffers
    auto& main_buffer = tl_row_buffer_[0];
    main_buffer.entries_.reserve(total_entries);

    for (size_t i = 1; i < nthreads_; ++i) {
      auto& second_buffer = tl_row_buffer_[i];
      // Shift the pointers in the second buffer by number
      // of entries in the main buffer
      const size_t ptr_offset = main_buffer.entries_.size();
      auto f_shift_ptr = [=](const size_t& p) { return p + ptr_offset; };

      // Remove the end pointer from main buffer, because it would be the same as the begin
      // of the next batch after shifting
      DASSERT_EQ(main_buffer.entry_ptr_.back(), second_buffer.entry_ptr_.front() + ptr_offset);
      main_buffer.entry_ptr_.pop_back();

      // Move the data into main buffer
      std::copy(boost::make_transform_iterator(second_buffer.entry_ptr_.begin(), f_shift_ptr),
                boost::make_transform_iterator(second_buffer.entry_ptr_.end(), f_shift_ptr),
                std::back_inserter(main_buffer.entry_ptr_));

      std::copy(std::make_move_iterator(second_buffer.entries_.begin()),
                std::make_move_iterator(second_buffer.entries_.end()),
                std::back_inserter(main_buffer.entries_));

      second_buffer.Reset();
      // Last pointer should always points to the end of the data entries.
      DASSERT_EQ(main_buffer.entry_ptr_.back(), main_buffer.entries_.size());
    }
    DASSERT_EQ(main_buffer.entry_ptr_.back(), main_buffer.entries_.size());
    DASSERT_EQ(main_buffer.Size(), end_row - begin_row);

    // Write the output
    out_.base_rowid = begin_row;
    out_.ind_ptr = BeginPtr(main_buffer.entry_ptr_);
    out_.data_ptr = BeginPtr(main_buffer.entries_);
    out_.size = main_buffer.Size();

    // Set current row to end of this batch
    current_row_ = end_row;
    utils::Assert(out_.size != 0, "MLDataBatchIterator");
  }

  // clear buffer
  void ResetBuffer(void) {
    for (auto& block: tl_row_buffer_) block.Reset();
  }

  // ---- ml data structure ---
  ml_data ml_data_;

  // ---- buffer size and threads ----
  size_t rows_per_batch_;
  size_t nthreads_;
  size_t num_batches_;

  // ---- ml data structure ---
  std::vector<ml_data_iterator> tl_iter_;

  // Thread local buffer
  std::vector<RowBlock> tl_row_buffer_;

  // Pointer to the next row to read
  size_t current_row_;

  // output data structure
  RowBatch out_;
};

/**************************************************************************/
/*                                                                        */
/*                    External Memory Column Batch Iterator               */
/*                                                                        */
/**************************************************************************/
/**
 * ColumnSparseBatch backed by SFrame
 *
 * The advantage of using SFrame backend is to allow compression for the value column, which is float.
 * The disadvantage is that the memory footprint is higher during decompression time.
 */
struct SFrameSparsePage {
 public:
  void load() {
    ASSERT_TRUE(is_saved);
    ASSERT_FALSE(in_memory);
    ASSERT_TRUE(offset.empty());
    ASSERT_TRUE(data.empty());

    in_memory = true;

    // Build the offset for the index_set
    offset.push_back(0);
    size_t total_length = 0;
    for (auto col_index: index_set) {
      size_t begin_row = offset_saved[col_index];
      size_t end_row = offset_saved[col_index + 1];
      size_t length = end_row - begin_row;
      offset.push_back(offset.back() + length);
      total_length += length;
    }
    data.resize(total_length);

    // Reorder index_set in ascending order to honor sequential access
    std::vector<size_t> reindex;
    for (size_t i = 0; i < index_set.size(); ++i) { reindex.push_back(i); }
    std::sort(reindex.begin(), reindex.end(), [&](size_t i, size_t j) { return index_set[i] < index_set[j]; });

    sframe_rows rows;
    std::unique_ptr<sframe_reader> reader = data_sframe.get_reader();
    // Read each column into data
    for (auto i : reindex) {
      size_t col_idx = index_set[i];
      size_t begin_row = offset_saved[col_idx];
      size_t end_row = offset_saved[col_idx + 1];

      reader->read_rows(begin_row, end_row, rows);
      auto& cols = rows.cget_columns();
      auto& index_col = *cols[0];
      auto& value_col = *cols[1];

      // fill in the struct
      size_t length = end_row - begin_row;
      size_t ptr = offset[i];
      for (size_t j = 0; j < length; ++j) {
        auto& entry = data[ptr];
        entry.index = (bst_uint)index_col[j];
        entry.fvalue = (bst_float)value_col[j];
        ++ptr;
      }
      DASSERT_EQ(ptr, offset[i+1]);
    };
  }
  void unload() {
    ASSERT_TRUE(is_saved);
    data.clear(); data.shrink_to_fit();
    offset.clear(); offset.shrink_to_fit();
    in_memory = false;
  }
  void set_index_set(const std::vector<bst_uint>& index_set) {
    this->index_set = index_set;
  }
  void save() {
    ASSERT_TRUE(in_memory);
    if (is_saved) return;
    data_sframe.open_for_write({"index", "value"}, {flex_type_enum::INTEGER, flex_type_enum::FLOAT}, "", 1);
    size_t nsegments = data_sframe.num_segments();
    parallel_for(0, data_sframe.num_segments(), [&](size_t segment_id) {
      auto out_iter = data_sframe.get_output_iterator(segment_id);
      size_t begin_row = data.size() * segment_id / nsegments;
      size_t end_row = data.size() * (segment_id +1)/ nsegments;
      std::vector<flexible_type> row_buffer(2);
      for (size_t i = begin_row; i < end_row; ++i) {
        row_buffer[0] = data[i].index;
        row_buffer[1] = data[i].fvalue;
        *out_iter++ = row_buffer;
      }
    });
    data_sframe.close();
    // save the offset
    offset_saved = offset;
    is_saved = true;
  }

  // in memory struct
  std::vector<size_t> offset;
  std::vector<size_t> offset_saved;
  std::vector<SparseBatch::Entry> data;
  std::vector<bst_uint> index_set;
  bool in_memory = true;

  // external memory struct
  sframe data_sframe;
  bool is_saved = false;
}; // end of SFrameSparsePage


typedef SFrameSparsePage DiskPageType;

// Convert a row batch into external column batch page.
void MakeColPage(const RowBatch &batch,
                 dense_bitset& row_mask,
                 size_t num_columns,
                 DiskPageType* pcol) {
  size_t nthread = thread::cpu_count();
  auto base_rowid = batch.base_rowid;
  utils::ParallelGroupBuilder<SparseBatch::Entry>
          builder(&(pcol->offset), &(pcol->data));
  builder.InitBudget(num_columns, nthread);
  bst_omp_uint ndata = static_cast<bst_uint>(batch.size);
  turi::parallel_for(0, ndata, [&](size_t i) {
    if (row_mask.get(i) == false) return;
    int tid = turi::thread::thread_id();
    RowBatch::Inst inst = batch[i];
    for (bst_uint j = 0; j < inst.length; ++j) {
      const SparseBatch::Entry &e = inst[j];
      builder.AddBudget(e.index, tid);
    }
  });
  builder.InitStorage();
  turi::parallel_for(0, ndata, [&](size_t i) {
    if (row_mask.get(i) == false) return;
    int tid = turi::thread::thread_id();
    RowBatch::Inst inst = batch[i];
    for (bst_uint j = 0; j < inst.length; ++j) {
      const SparseBatch::Entry &e = inst[j];
      builder.Push(e.index,
                   SparseBatch::Entry(base_rowid + i, e.fvalue),
                   tid);
    }
  });
  // sort columns
  bst_omp_uint ncol = static_cast<bst_omp_uint>(num_columns);
  turi::parallel_for (0, ncol, [&](size_t i) {
    if (pcol->offset[i] < pcol->offset[i + 1]) {
      std::sort(BeginPtr(pcol->data) + pcol->offset[i],
                BeginPtr(pcol->data) + pcol->offset[i + 1],
                SparseBatch::Entry::CmpValue);
    }
  });
  pcol->save();
  pcol->unload();
}

/**
 * ColumnBatchIterator backed by DiskPageType 
 */
class ColBatchIter: public utils::IIterator<ColBatch> {
 public:
  ColBatchIter(void) { 
    is_inited = false;
    num_io_threads = std::max<size_t>(2, (thread::cpu_count() / 2));
    pool = std::make_shared<thread_pool>(num_io_threads);
  }
  ~ColBatchIter(void) { 
    this->Clear(); 
    pool->join();
  }
  void BeforeFirst(void) {
    ASSERT_TRUE(is_inited);
    for (auto& page: pages_) {
      page.unload();
      page.set_index_set(col_index_);
    }
    async_loaders_.clear();
    async_loaders_.resize(pages_.size(), nullptr);
    current_page_ = 0;
  }
  bool Next(void) {
    ASSERT_TRUE(is_inited);
    if (current_page_ == pages_.size()) return false;
    // Unload previous page and load new page
    if (current_page_ > 0) {
      pages_[current_page_-1].unload();
    }
    // Wait for current page
    if (async_loaders_[current_page_] == nullptr) {
      async_loaders_[current_page_].reset(new PageLoader(pages_[current_page_], *pool));
    }
    async_loaders_[current_page_]->wait();

    // Prefetch with half cpu count, each page is bounded by 512MB
    for (size_t k = 1; k <= num_io_threads; ++k) {
      size_t next_page = current_page_ + k;
      if (next_page < pages_.size() && async_loaders_[next_page] == nullptr) {
        async_loaders_[next_page].reset(new PageLoader(pages_[next_page], *pool));
      }
    }
    // Fill in the struct with current page data
    col_data_.resize(col_index_.size(), SparseBatch::Inst(NULL, 0));
    auto& page = pages_[current_page_];
    for (size_t i = 0; i < col_index_.size(); ++i) {
      col_data_[i] = SparseBatch::Inst
          (BeginPtr(page.data) + page.offset[i],
           static_cast<bst_uint>(page.offset[i + 1] - page.offset[i]));
    }
    batch_.size = col_index_.size();
    batch_.col_index = BeginPtr(col_index_);
    batch_.col_data = BeginPtr(col_data_);
    ++current_page_;
    return true;
  }
  const ColBatch &Value(void) const {
    ASSERT_TRUE(is_inited);
    return batch_;
  }
  void Clear(void) {
    async_loaders_.clear();
    col_index_.clear();
    col_data_.clear();
    pages_.clear(); 
  }
  void SetPages(std::vector<DiskPageType>& pages_) {
    this->pages_ = std::move(pages_);
    is_inited = true;
  }
  void SetIndexSet(const std::vector<bst_uint>& col_index_) {
    this->col_index_ = col_index_;
  }
 private:
  class PageLoader {
   public:
    PageLoader(DiskPageType& page, thread_pool& pool) : page(page), done(false) {
      pool.launch(
        [this]() {
          try {
            this->page.load();
          } catch (...) {
            exception = std::current_exception();
          }
          this->signal_done();
        });
    }
    ~PageLoader() {
      try {
        wait();
      } catch(...) {
      }
    }
    void signal_done() {
      std::lock_guard<mutex> lock(mtx);
      done = true;
      cv.notify_one();
    }
    void wait() {
      std::unique_lock<mutex> lock(mtx);
      while (done == false) {
        cv.wait(lock);
      }
      if (exception) {
        throw("Canceled by user");
      }
    }
   private:
    DiskPageType& page;
    turi::conditional cv;
    turi::mutex mtx;
    std::exception_ptr exception;
    bool done = false;
  };
 private:
  bool is_inited = false;
  size_t num_io_threads = 2;
  std::shared_ptr<thread_pool> pool;
  // the selected column indices 
  std::vector<bst_uint> col_index_;
  // column sparse pages
  std::vector<DiskPageType> pages_;
  std::vector<std::shared_ptr<PageLoader>> async_loaders_;
  // pointer to current column page
  size_t current_page_;
  // column content
  std::vector<ColBatch::Inst> col_data_;
  // temporal space for batch
  ColBatch batch_;
}; // end of ColBatchIterator


/*!
 * \brief sparse matrix that support column access, CSC
 */
class DiskPagedFMatrix: public IFMatrix {
 public:
  typedef SparseBatch::Entry Entry;
  /*! \brief constructor */
  DiskPagedFMatrix(utils::IIterator<RowBatch> *iter,
                     const learner::MetaInfo &info,
                     size_t num_batches) : info(info), num_batches(num_batches) {
    this->iter_ = iter;
  }
  // destructor
  virtual ~DiskPagedFMatrix(void) {
    if (iter_ != NULL) delete iter_;
  }
  /*! \return whether column access is enabled */
  virtual bool HaveColAccess(void) const {
    return col_size_.size() != 0;
  }
  /*! \brief get number of colmuns */
  virtual size_t NumCol(void) const {
    utils::Check(this->HaveColAccess(), "NumCol:need column access");
    return col_size_.size();
  }
  /*! \brief get number of buffered rows */
  virtual const std::vector<bst_uint> &buffered_rowset(void) const {
    return buffered_rowset_;
  }
  /*! \brief get column size */
  virtual size_t GetColSize(size_t cidx) const {
    return col_size_[cidx];
  }
  /*! \brief get column density */
  virtual float GetColDensity(size_t cidx) const {
    size_t nmiss = num_buffered_row_ - (col_size_[cidx]);
    return 1.0f - (static_cast<float>(nmiss)) / num_buffered_row_;
  }
  virtual void InitColAccess(const std::vector<bool> &enabled,
                             float pkeep, size_t max_row_perbatch) {
    if (this->HaveColAccess()) return;
    // The parameter "enabled" is deprecated. 
    // Assume enabled is all true that we are using all columns.
    ASSERT_TRUE(std::all_of(enabled.begin(), enabled.end(), [](bool x) { return x; }));
    this->InitColData(pkeep, max_row_perbatch);

    // Report summary of column density
    size_t ncol = NumCol();
    std::vector<float> col_density; 
    for (size_t i = 0; i < ncol; ++i) { col_density.push_back(GetColDensity(i)); }
    std::sort(col_density.begin(), col_density.end());
    std::vector<size_t> quantiles {0, ncol/4, ncol/2, ncol*3/4, ncol-1};
    std::stringstream ss;
    ss << "Feature density quantile (0%, 25%, 50%, 100%): ";
    for (auto i : quantiles) ss << col_density[i] << " ";
    logstream(LOG_INFO) << "Number of features after expand: " 
                            << col_density.size() << std::endl;
    logstream(LOG_INFO) << ss.str() << std::endl;
  }
  /*!
   * \brief get the row iterator associated with FMatrix
   */
  virtual utils::IIterator<RowBatch>* RowIterator(void) {
    iter_->BeforeFirst();
    return iter_;
  }
  /*!
   * \brief get the column based  iterator
   */
  virtual utils::IIterator<ColBatch>* ColIterator(void) {
    size_t ncol = this->NumCol();
    col_index_.clear();
    for (size_t i = 0; i < ncol; ++i) {
      col_index_.push_back(static_cast<bst_uint>(i));
    }
    col_iter_.SetIndexSet(col_index_);
    col_iter_.BeforeFirst();
    return &col_iter_;
  }
  /*!
   * \brief colmun based iterator
   */
  virtual utils::IIterator<ColBatch> *ColIterator(const std::vector<bst_uint> &fset) {
    size_t ncol = this->NumCol();
    col_index_.clear();
    for (size_t i = 0; i < fset.size(); ++i) {
      if (fset[i] < ncol) col_index_.push_back(fset[i]);
    }
    col_iter_.SetIndexSet(col_index_);
    col_iter_.BeforeFirst();
    return &col_iter_;
  }
 protected:
  /*!
   * \brief intialize column data
   * \param pkeep probability to keep a row
   */
  void InitColData(float pkeep, size_t max_row_perbatch) {
    // Init data structures 
    buffered_rowset_.clear();
    buffered_rowset_.reserve(info.num_row());
    col_size_.clear();
    col_size_.resize(info.num_col(), 0);

    bool skip_sample = (pkeep == 1.0f);

    // Iterate row batches, and convert to column batch pages
    std::vector<DiskPageType> col_pages;
    iter_->BeforeFirst();
    size_t batch_id = 0;
    while (iter_->Next()) {
      ++batch_id;
      logstream(LOG_PROGRESS) << "Create disk column page " << batch_id << "/" << num_batches << std::endl;
      auto& rowbatch = iter_->Value();
      size_t base_rowid = rowbatch.base_rowid;
      dense_bitset row_mask(rowbatch.size);
      for (size_t i = 0; i < rowbatch.size; ++i) {
        if (skip_sample || ::xgboost::random::SampleBinary(pkeep)) {
          row_mask.set_bit(i);
          buffered_rowset_.push_back(base_rowid + i);
        }
      }
      col_pages.push_back(DiskPageType());
      auto& new_page = col_pages.back();
      MakeColPage(rowbatch, row_mask, info.num_col(), &(new_page));
      for (size_t i = 0; i < info.num_col(); ++i) {
        col_size_[i] += new_page.offset_saved[i + 1] - new_page.offset_saved[i];
      }
    }
    col_iter_.SetPages(col_pages);
    num_buffered_row_ = buffered_rowset_.size();
  }
 private:
  // shared meta info with DMatrix
  const learner::MetaInfo &info;
  // row iterator
  utils::IIterator<RowBatch> *iter_;
  /*! \brief list of row index that are buffered */
  std::vector<bst_uint> buffered_rowset_;
  // number of buffered rows
  size_t num_buffered_row_;
  // count for column data
  std::vector<size_t> col_size_;
  // internal column index for output
  std::vector<bst_uint> col_index_;
  // internal batch col iterator
  ColBatchIter col_iter_;
  // number of batches in both row and col batch iterator
  size_t num_batches;
}; // end of DiskPagedFMatrix


DMatrixMLData::DMatrixMLData(const ml_data &data,
  flexible_type class_weights,
  storage_mode_enum storage_mode,
  size_t num_batches) : DMatrix(kMagic) {

  auto metadata = data.metadata();
  info.info.num_row = data.size();
  info.info.num_col = data.metadata()->num_dimensions();

  // Class weights
  std::map <size_t, float> _class_weights;
  bool has_class_weights = (class_weights != flex_undefined());
  bool is_categorical = data.has_target() && metadata->target_is_categorical();
  if (has_class_weights){
    for(const auto& kvp: class_weights.get<flex_dict>()){
      size_t index = metadata->target_indexer()
                             ->immutable_map_value_to_index(kvp.first);
      DASSERT_TRUE(index != size_t(-1)); 
      _class_weights[index] = kvp.second.get<flex_float>();
    }
  }
  // Target.
  if (data.has_target()) {
    // Set size
    info.labels.resize(data.num_rows());
    if (has_class_weights) {
      info.weights.resize(data.num_rows());
    }
    this->num_classes_ = data.metadata()->target_column_size();
    // Fill the target
    in_parallel([&](size_t thread_idx, size_t num_threads){
      flexible_type true_value, predicted_value;
      size_t target_index = 0;
      for(auto it = data.get_iterator(thread_idx, num_threads); 
                                           !it.done(); ++it) {
        if (is_categorical) {
          target_index = it->target_index();
          info.labels[it.row_index()] = target_index;
          if (has_class_weights) {
            // target_index may not exist in _class_weights which is filled up 
            // based on training data. Default weight is 1 for new class.
            if (_class_weights.count(target_index) != 0) {
              info.weights[it.row_index()] = _class_weights.at(target_index);
            } else {
              info.weights[it.row_index()] = 1.0;
            }
          }
        } else {
          info.labels[it.row_index()] = it->target_value();
        }
      }
    });
  }

  /**
   * Construct iterator based on storage mode
   */
  size_t max_row_per_batch = (size_t)(-1);

  // Auto infer batch size based on memory limit
  const size_t cache_size_per_batch = 512 * 1024 * 1024UL; // 512M per batch

  // Get the system memory limit
  size_t memory_limit_mb = (size_t)(-1);
  char* envval = getenv("TURI_MEMORY_LIMIT_IN_MB");
  if (envval != nullptr) {
    memory_limit_mb = atoi(envval);
  } else {
    memory_limit_mb = total_mem() / (1024 * 1024UL);
  }

  if (num_batches == 0) {
    const size_t max_row_size_in_bytes = data.max_row_size() * sizeof(RowBatch::Entry);
    max_row_per_batch = cache_size_per_batch / (max_row_size_in_bytes);
    num_batches = (data.num_rows() + max_row_per_batch - 1) / max_row_per_batch;
    logstream(LOG_INFO) << "Auto tune batch size... Memory limit (MB): " << memory_limit_mb << "MB" 
                        << std::endl;
    logstream(LOG_INFO) << " Max cache per batch: " << cache_size_per_batch/float(1024 * 1024) << "MB"
                        << " Max row size: " << max_row_size_in_bytes << "B" << std::endl;
    logstream(LOG_INFO) << "Number of batches: "  << num_batches
                        << " Max row per batch: " << max_row_per_batch 
                        << std::endl; 
  } else {
    max_row_per_batch = (data.num_rows() + num_batches - 1) / num_batches;
    logstream(LOG_INFO) << "Fixed number of batches: "  << num_batches
                        << " Max row per batch: " << max_row_per_batch 
                        << std::endl;
  }

  size_t num_io_threads = std::max<size_t>(2, (thread::cpu_count() / 2));
  MLDataBatchIterator* it = new MLDataBatchIterator(data, max_row_per_batch, num_io_threads);

  size_t estmate_memory_in_mb = (num_batches * cache_size_per_batch / (1024 * 1024UL));
  bool exceeds_memory_limit = estmate_memory_in_mb > (memory_limit_mb / 4.0);
  // Decide storage mode
  if (storage_mode == storage_mode_enum::IN_MEMORY ||
     /* heuristic*/
     (storage_mode == storage_mode_enum::AUTO && !exceeds_memory_limit)) {
    logstream(LOG_INFO) << "Use in memory storage mode" << std::endl;
    storage_mode = storage_mode_enum::IN_MEMORY;
    auto fmat = new FMatrixS(it, this->info);
    fmat_ = fmat;
  } else {
    logstream(LOG_INFO) << "Use external memory storage mode" << std::endl;
    storage_mode = storage_mode_enum::EXT_MEMORY;
    auto fmat = new DiskPagedFMatrix(it, this->info, num_batches);
    /* Switch to use original xgboost's FMatrixPage backend */
    // auto fmat = new FMatrixPage(it, this->info);
    // auto temp_file = get_temp_name();
    // fmat->set_cache_file(temp_file);
    fmat_ = fmat;
  }

  use_extern_memory_ = storage_mode == storage_mode_enum::EXT_MEMORY;
  logstream(LOG_INFO) << "Number of columns = " << info.num_col()
                      << " Number of rows = " << info.num_row() << std::endl;
  logstream(LOG_INFO) << "Number of batches =  " << num_batches
                      << " Batch size = " << max_row_per_batch << std::endl;
  if (use_extern_memory_) {
    logstream(LOG_PROGRESS) << "External memory mode: " << num_batches <<  " batches" << std::endl;
  }
}

DMatrixMLData::~DMatrixMLData(void) {
  delete fmat_;
}

::xgboost::IFMatrix *DMatrixMLData::fmat(void) const {
  return fmat_;
}


}  // namespace xgboost
}  // namespace supervised
}  // namespace turi
