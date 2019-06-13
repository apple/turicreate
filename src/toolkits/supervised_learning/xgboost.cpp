/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifdef CHECK
#undef CHECK
#endif
#ifdef MIN
#undef MIN
#endif
#ifdef MAX
#undef MAX
#endif

#include <toolkits/supervised_learning/xgboost.hpp>
#include <toolkits/supervised_learning/xgboost_iterator.hpp>

#include <limits>
#include <sstream>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>

#include <core/logging/logger.hpp>
#include <timer/timer.hpp>
#include <core/storage/fileio/temp_files.hpp>
#include <core/storage/fileio/sanitize_url.hpp>

// CoreML
#include <toolkits/coreml_export/xgboost_exporter.hpp>


// sframe
#include <core/storage/sframe_data/algorithm.hpp>
#include <core/storage/sframe_interface/unity_sarray.hpp>

// xgboost
#include <xgboost/src/learner/learner-inl.hpp>
#include <xgboost/src/io/simple_dmatrix-inl.hpp>

// Toolkits
#include <toolkits/supervised_learning/supervised_learning_utils-inl.hpp>
#include <toolkits/evaluation/metrics.hpp>

namespace turi {
namespace supervised {
namespace xgboost {

using namespace ::xgboost;
using namespace ::xgboost::io;
using namespace ::xgboost::learner;
typedef std::shared_ptr<evaluation::supervised_evaluation_interface> evalptr;
typedef std::shared_ptr<learner::IEvaluator> xgboost_evalptr;

/**************************************************************************/
/*                                                                        */
/*                               IO Adapter                               */
/*                                                                        */
/**************************************************************************/
// adapter, used to serialization of xgboost
struct ArcStreamInAdapter: public utils::IStream {
  turi::iarchive &iarc;
  ArcStreamInAdapter(turi::iarchive &arc) : iarc(arc){}
  virtual size_t Read(void *ptr, size_t size) {
    iarc.read((char*)ptr, size);
    return 1;
  }
  // write is ensured not being called when using this adapter
  virtual void Write(const void *ptr, size_t size) {
    utils::Assert(false, "ArcStreamInAdapter: write not implemented");
  }
};
struct ArcStreamOutAdapter: public utils::IStream {
  turi::oarchive &oarc;
  ArcStreamOutAdapter(turi::oarchive &arc) : oarc(arc) {}
  // read is ensured not being called when using this adapter
  virtual size_t Read(void *ptr, size_t size) {
    utils::Assert(false, "ArcStreamOutAdapter: read not implemented");
    return 0;
  }
  virtual void Write(const void *ptr, size_t size){
    oarc.write((const char*)ptr, size );
  }
};


std::vector<RowBatch::Entry> make_row_batch(
    const flex_dict& row,
    std::shared_ptr<ml_metadata> metadata,
    const ml_missing_value_action& na_enum) {

  std::vector<RowBatch::Entry> ret;
  ml_data_row_reference::from_row(metadata, row, na_enum).unpack(
      [&](ml_column_mode mode, size_t column_index,
          size_t feature_index, double value,
          size_t index_size, size_t index_offset) {
        // skip NAN as missing values
        if(UNLIKELY(feature_index >= index_size || std::isnan(value)))
          return;
        size_t idx = index_offset + feature_index;
        DASSERT_GE(idx,  0);
        ret.push_back(RowBatch::Entry(idx ,value));
      },
      [&](ml_column_mode mode, size_t column_index, size_t index_size) { }
                                                                 );
  return ret;
}

DMatrixSimple make_simple_dmatrix(
    const std::vector<flexible_type>& rows,
    std::shared_ptr<ml_metadata> metadata,
    const ml_missing_value_action& na_enum) {

  DMatrixSimple ret;
  for (const auto& value: rows) {
    if (value.get_type() != flex_type_enum::DICT) {
      log_and_throw(
          "TypeError: Expecting dictionary as input type for each example.");
    }
    ret.AddRow(make_row_batch(value.get<flex_dict>(), metadata, na_enum));
  }
  return ret;
}

void MakeFeatMap(utils::FeatMap &featmap,
                 std::shared_ptr<ml_metadata> & metadata) {
  size_t fbase = 0;
  for (size_t col = 0; col < metadata->num_columns(); ++col) {

    switch (metadata->column_type(col)) {
      case flex_type_enum::STRING:
        // fname = column_name=value, e.g. gender=F
        for (size_t offset = 0; offset < metadata->index_size(col); ++offset) {
          std::string fname =
              (metadata->column_name(col) + "=" +
               metadata->indexer(col)->map_index_to_value(offset).to<std::string>());
          featmap.PushBack(fbase + offset, fname.c_str(), "i");
        }
        break;

      case flex_type_enum::INTEGER:

        // fname = column_name e.g. age < 25
        for (size_t offset = 0; offset < metadata->index_size(col); ++offset) {
          featmap.PushBack(fbase, metadata->column_name(col).c_str(), "int");
        }
        break;

      case flex_type_enum::FLOAT:
      case flex_type_enum::DICT:
      case flex_type_enum::LIST: /* Lists function like a dict with all values = 1. */
      case flex_type_enum::VECTOR:
      case flex_type_enum::ND_VECTOR:

        // fname = column_name[index] e.g. prob[1] > 0.5
        for (size_t offset = 0; offset < metadata->index_size(col); ++offset) {
          featmap.PushBack(fbase + offset, metadata->feature_name(col, offset, true).c_str(), "q");
        }

        break;
      default:
        ASSERT_MSG(
            false,
            "Internal error: type not handled in xgboost switch statement.");
    }

    fbase += metadata->index_size(col);
  }
}


/**************************************************************************/
/*                                                                        */
/*                       Early Stop and Checkpoint                        */
/*                                                                        */
/**************************************************************************/
// Take snapshot of the boost learner state given iteration.
struct early_stopping_checkpointer {
  typedef ::xgboost::learner::BoostLearner raw_model_type;
  typedef std::shared_ptr<raw_model_type> model_type;

  early_stopping_checkpointer(size_t max_models_to_keep, bool
                              tracking_max_score, size_t early_stopping_rounds):
      max_models_to_keep(max_models_to_keep),
      tracking_max_score(tracking_max_score),
      early_stopping_rounds(early_stopping_rounds) {
    if (tracking_max_score) {
      best_score = std::numeric_limits<float>::min();
    } else {
      best_score = std::numeric_limits<float>::max();
    }
  }
  void add(size_t iter, model_type model, float score) {
    std::string tmp_file = get_temp_name();
    bool save_with_pbuffer = false;
    // save model to tmp file
    model->SaveModel(tmp_file.c_str(), save_with_pbuffer);
    // load model from tmp file
    auto model_copy = std::make_shared<raw_model_type>();
    model_copy->LoadModel(tmp_file.c_str());
    model_queue.push_back(std::make_pair(iter, model_copy));
    if (model_queue.size() > max_models_to_keep) {
      model_queue.pop_front();
    }
    // update best score and iter
    if (tracking_max_score && (score > best_score)) {
      best_score = score;
      best_iter = iter;
    } else if (!tracking_max_score && (score < best_score)) {
      best_score = score;
      best_iter = iter;
    }
  }

  model_type get_model_at_iteration(size_t iter) {
    auto ret = std::find_if(model_queue.begin(), model_queue.end(),
                            [&](const std::pair<size_t, model_type>& v) { return v.first == iter; });
    if (ret != model_queue.end()) {
      return ret->second;
    } else {
      throw("Cannot find model at given iteration");
    }
  }

  model_type get_best_model() {
    return get_model_at_iteration(best_iter);
  }

  size_t get_best_iter() {
    return best_iter;
  }

  bool need_stop(size_t iter) {
    return (iter - best_iter) >= early_stopping_rounds;
  }

  std::deque<std::pair<size_t, model_type>> model_queue;
  size_t max_models_to_keep;
  bool tracking_max_score;
  float best_score;
  size_t best_iter = 0;
  size_t early_stopping_rounds = 100;
};

size_t xgboost_model::_get_early_stopping_rounds(bool has_validation_data) {
  size_t early_stopping_rounds_ = 0;
  if (this->state.count("early_stopping_rounds")) {
    flexible_type tmp = variant_get_value<flexible_type>(state.at("early_stopping_rounds"));
    if (tmp.get_type() != flex_type_enum::UNDEFINED) {
      early_stopping_rounds_ = tmp;
    }
  }
  if (early_stopping_rounds_> 0) {
    if (tracking_metrics.size() == 0) {
      log_and_throw("Tracking metric must be specified for early stop");
    }
    if (!has_validation_data) {
      log_and_throw("Validation set must be specified for early stop. \
                     If using \"auto\" validation, please increase the size of training data.");
    }
  }
  return early_stopping_rounds_;
}

void xgboost_model::_checkpoint(const std::string& path) {
  logprogress_stream << "Checkpointing to " << sanitize_url(path) << std::endl;
  dir_archive dir;
  dir.open_directory_for_write(path);
  dir.set_metadata("contents", "model");
  oarchive oarc(dir);
  bool save_booster_prediction_buffer = false;
  _save(oarc, save_booster_prediction_buffer);
  dir.close();
}

void xgboost_model::_restore_from_checkpoint(const std::string& path) {
  dir_archive dir;
  dir.open_directory_for_read(path);
  iarchive iarc(dir);
  auto new_option_values = this->options.current_option_values();
  load_version(iarc, XGBOOST_MODEL_VERSION);
  dir.close();

  // If user input a parameter different from the the checkpoint, warn that we will ignore the new parameter
  for (auto kv: new_option_values) {
    if ((kv.first != "model_checkpoint_path") && (kv.first != "resume_from_checkpoint")) {
      if (this->options.value(kv.first) != kv.second) {
        logprogress_stream << "Warning: ignoring provided value of " << kv.first
                           << " which is different from the model checkpoint" << std::endl;
      }
    }
  }
  this->options.set_option("resume_from_checkpoint", path);
  this->options.set_option("model_checkpoint_path", new_option_values.at("model_checkpoint_path"));
}

/**************************************************************************/
/*                                                                        */
/*                            Helper functions                            */
/*                                                                        */
/**************************************************************************/
std::vector<float> fast_evaluate(const std::vector<float>& preds,
                                 const learner::MetaInfo& info,
                                 std::vector<xgboost_evalptr>& evaluators) {
  bool distributed = false;
  std::vector<float> ret;
  for (auto& e : evaluators) {
    float v = e->Eval(preds, info, distributed);
    ret.push_back(v);
  }
  return ret;
}


/**
 * Transform the result of an error based evaluator to accuracy based.
 */
template<typename ErrorEvaluator>
class ErrorToAccuracyEvaluator: public ErrorEvaluator {
 public:
  virtual float Eval(const std::vector<float> &preds,
                     const MetaInfo &info,
                     bool distributed) const {
    return 1.0f - ErrorEvaluator::Eval(preds, info, distributed);
  }
};
typedef ErrorToAccuracyEvaluator<EvalError> EvalBinaryAccuracy;
typedef ErrorToAccuracyEvaluator<EvalMatchError> EvalMultiClassAccuracy;

/**
 * Max Error Evaluator
 */
class EvalMaxError: public IEvaluator {
 public:
  virtual float Eval(const std::vector<float>& preds,
                     const MetaInfo& info,
                     bool distributed = false) const {
    const bst_omp_uint ndata = static_cast<bst_omp_uint>(info.labels.size());
    std::vector<float> max_per_thread(thread::cpu_count(), 0.0);
    // #pragma omp parallel for reduction(+: sum, wsum) schedule(static)
    // for (bst_omp_uint i = 0; i < ndata; ++i) {
    turi::parallel_for(0, ndata, [&](size_t i) {
        const float wt = info.GetWeight(i);
        float val = std::fabs(info.labels[i] - preds[i]) * wt;
        max_per_thread[thread::thread_id()] =
            std::max<float>(val, max_per_thread[thread::thread_id()]);
      });
    float max_overall = max_per_thread[0];
    for (auto m : max_per_thread) {
      max_overall = std::max<float>(max_overall, m);
    }
    float dat[1]; dat[0] = max_overall;
    if (distributed) {
      rabit::Allreduce<rabit::op::Max>(dat, 1);
    }
    return dat[0];
  }
  virtual const char* Name(void) const {
    return "max_error";
  }
};

/**
 * Helper function: create an xgboost IEvalator from metric name.
 */
xgboost_evalptr get_fast_evaluator(std::string name, size_t num_classes) {
  if (name == "log_loss") {
    if (num_classes == 2) {
      return std::make_shared<EvalLogLoss>();
    } else {
      return std::make_shared<EvalMultiLogLoss>();
    }
  } else if (name == "auc") {
    if (num_classes == 2){
      return std::make_shared<EvalAuc>();
    } else {
      log_and_throw(std::string("Multiclass AUC is not supported as tracking metric"));
    }
  } else if (name == "accuracy") {
    if (num_classes == 2) {
      return std::make_shared<EvalBinaryAccuracy>();
    } else {
      return std::make_shared<EvalMultiClassAccuracy>();
    }
  } else if (name == "rmse") {
    return std::make_shared<EvalRMSE>();
  } else if (name == "max_error") {
    return std::make_shared<EvalMaxError>();
  }  else {
    log_and_throw(std::string("Invalid tracking metric: ") + name);
  }
}

/**
 * Given a user input of tracking metric argument, return a vector of parsed metric names.
 *
 * User input can be string or list[string]. If the type is string and value is "auto", return
 * the default_metrics.
 *
 * The returned vector contains zero or more valid metric names.
 */
std::vector<std::string> parse_tracking_metric(const flexible_type& input_metric,
                                               const std::vector<std::string>& default_metrics,
                                               bool is_classifier) {
  static std::set<std::string> supported_regression_metrics{ "rmse", "max_error"};
  static std::set<std::string> supported_classifier_metrics{"accuracy", "log_loss", "auc"};
  std::vector<std::string> ret;

  // Default
  if (input_metric.get_type() == flex_type_enum::UNDEFINED) {
    return ret;
  } else if (input_metric == "auto") {
    return default_metrics;
  }

  if (input_metric.get_type() == flex_type_enum::STRING) {
    ret.push_back(input_metric);
  } else if (input_metric.get_type() == flex_type_enum::LIST) {
    for (auto& v : input_metric.get<flex_list>()) {
      if (v.get_type() != flex_type_enum::STRING) {
        throw(turi::bad_cast("Invalid type for metric. Expect string or list[string]"));
      }
      ret.push_back((std::string)v);
    }
  } else {
    throw(turi::bad_cast("Invalid type for metric. Expect string or list[string]"));
  }

  std::vector<std::string> filtered_ret;
  for (auto& i : ret) {
    if ((is_classifier && supported_classifier_metrics.count(i) == 0) ||
        (!is_classifier && supported_regression_metrics.count(i) == 0)) {
      logprogress_stream << "WARNING: Ignore unsupported tracking metric " << i << std::endl;
    } else {
      filtered_ret.push_back(i);
    }
  }
  return filtered_ret;
}

/**
 * Transform raw prediction values to the output type.
 * \param preds holds probability or margin
 * \param output_type enum for prediction output type
 * \param num_classes
 */
std::shared_ptr< sarray<flexible_type> > transform_prediction(const std::vector<float>& preds,
                                                              prediction_type_enum output_type,
                                                              size_t num_classes,
                                                              std::shared_ptr<ml_metadata> ml_mdata) {
  auto sa = std::make_shared<sarray<flexible_type>>();
  sa->open_for_write();
  if (num_classes == 0) {
    // Regression
    sa->set_type(flex_type_enum::FLOAT);
    turi::copy(preds.begin(), preds.end(), *sa);
  } else if (num_classes == 2) {
    //  Binary classification
    switch(output_type) {
      case prediction_type_enum::PROBABILITY:
      case prediction_type_enum::MARGIN:
        {
          sa->set_type(flex_type_enum::FLOAT);
          turi::copy(preds.begin(), preds.end(), *sa);
          break;
        }
      case prediction_type_enum::CLASS_INDEX:
        {
          sa->set_type(flex_type_enum::INTEGER);
          auto transform_fn = [](const float& x) { return x >= 0.5; };
          turi::copy(boost::make_transform_iterator(preds.begin(), transform_fn),
                     boost::make_transform_iterator(preds.end(), transform_fn),
                     *sa);
          break;
        }
      case prediction_type_enum::NA:
      case prediction_type_enum::CLASS:
        {
          sa->set_type(ml_mdata->target_column_type());
          auto& target_indexer = ml_mdata->target_indexer();
          auto transform_fn = [=](const float& x) {
            return target_indexer->map_index_to_value(x >= 0.5);
          };
          turi::copy(boost::make_transform_iterator(preds.begin(), transform_fn),
                     boost::make_transform_iterator(preds.end(), transform_fn),
                     *sa);
          break;
        }
      case prediction_type_enum::PROBABILITY_VECTOR:
        {
          sa->set_type(flex_type_enum::VECTOR);
          auto transform_fn = [](const float& x) {
            return flex_vec{1.0-x, x};
          };
          turi::copy(boost::make_transform_iterator(preds.begin(), transform_fn),
                     boost::make_transform_iterator(preds.end(), transform_fn),
                     *sa);
          break;
        }
      default:
        log_and_throw("Unexpected output type");
    }
    // end if binary classifier
  } else {
    ASSERT_MSG(preds.size() % num_classes == 0, "Unexpected prediction size");
    // Multiclass classifier
    switch(output_type) {
      case prediction_type_enum::MAX_PROBABILITY:
        {
          sa->set_type(flex_type_enum::FLOAT);
          std::vector<double> max_probability(preds.size() / num_classes);
          auto transform_to_max_prob = [&](const float& x) {
            auto begin_iter = &x;
            auto end_iter = &x + num_classes;
            return *(std::max_element(begin_iter, end_iter));
          };
          parallel_for(0, max_probability.size(), [&](size_t idx) {
              size_t idx_in_preds = idx * num_classes;
              max_probability[idx] = transform_to_max_prob(preds[idx_in_preds]);
            });
          turi::copy(max_probability.begin(), max_probability.end(), *sa);
          break;
        }
      case prediction_type_enum::NA:
      case prediction_type_enum::CLASS_INDEX:
      case prediction_type_enum::CLASS:
        {
          // Store the index of the best class
          std::vector<size_t> class_index_array(preds.size() / num_classes);
          auto transform_to_class_idx = [&](const float& x) {
            auto begin_iter = &x;
            auto end_iter = &x + num_classes;
            auto max_iter = std::max_element(begin_iter, end_iter);
            return (max_iter - begin_iter);
          };
          parallel_for(0, class_index_array.size(), [&](size_t idx) {
              size_t idx_in_preds = idx * num_classes;
              class_index_array[idx] = transform_to_class_idx(preds[idx_in_preds]);
            });

          // Write to output
          if (output_type == prediction_type_enum::CLASS_INDEX) {
            sa->set_type(flex_type_enum::INTEGER);
            turi::copy(class_index_array.begin(), class_index_array.end(), *sa);
          } else {
            sa->set_type(ml_mdata->target_column_type());
            auto target_indexer = ml_mdata->target_indexer();
            auto transform_to_class_name = [&](const size_t& x) {
              return target_indexer->map_index_to_value(x);
            };
            turi::copy(boost::make_transform_iterator(class_index_array.begin(), transform_to_class_name),
                       boost::make_transform_iterator(class_index_array.end(), transform_to_class_name),
                       *sa);
          }
          break;
        }
      case prediction_type_enum::PROBABILITY_VECTOR:
        {
          sa->set_type(flex_type_enum::VECTOR);
          size_t num_segments = sa->num_segments();
          size_t num_examples = preds.size() / num_classes;
          parallel_for(0, num_segments, [&](size_t segment_id) {
              auto out_iter = sa->get_output_iterator(segment_id);
              size_t example_begin =  num_examples * segment_id / num_segments;
              size_t example_end = num_examples * (segment_id + 1) / num_segments;

              std::vector<double> prob_vec(num_classes, 0.0);
              auto begin_iter = preds.begin() + example_begin * num_classes;
              auto end_iter = preds.begin() + example_end * num_classes;
              while (begin_iter != end_iter) {
                std::copy(begin_iter, begin_iter + num_classes, prob_vec.begin());
                *out_iter = prob_vec;
                begin_iter += num_classes;
                ++out_iter;
              }
            });
          break;
        }
      default:
        log_and_throw("Unexpected output type");
    }
    // end if multiclass classifier
  }
  sa->close();
  return sa;
}

/**
 * Transform raw prediction values to the topk output type.
 * \param preds holds probability or margin
 * \param output_type enum for prediction output type
 * \param num_classes
 */
sframe transform_prediction_topk(const std::vector<float>& preds,
                                 const std::string& output_type,
                                 size_t topk,
                                 size_t num_classes,
                                 std::shared_ptr<ml_metadata> ml_mdata) {
  // Select the output column type
  auto output_type_enum = prediction_type_enum_from_name(output_type);
  flex_type_enum output_column_type = flex_type_enum::FLOAT;
  switch(output_type_enum) {
    case prediction_type_enum::RANK: output_column_type = flex_type_enum::INTEGER; break;
    case prediction_type_enum::MARGIN:
    case prediction_type_enum::PROBABILITY:  output_column_type = flex_type_enum::FLOAT; break;
    default: log_and_throw("Unexpected output type");
  }

  auto target_indexer = ml_mdata->target_indexer();
  size_t stride = num_classes == 2 ? 1 : num_classes;

  // Make SFrame
  std::vector<std::string> col_names {"id", "class", output_type};
  std::vector<flex_type_enum> col_types {flex_type_enum::INTEGER,
        ml_mdata->target_column_type(),
        output_column_type};
  sframe sf;
  sf.open_for_write(col_names, col_types, "");
  size_t num_segments = sf.num_segments();
  size_t num_examples = preds.size() / stride;

  parallel_for(0, num_segments, [&](size_t segment_id) {
      auto out_iter = sf.get_output_iterator(segment_id);
      size_t example_begin = num_examples * segment_id / num_segments;
      size_t example_end = num_examples * (segment_id + 1) / num_segments;

      // Temporary variables
      std::vector<flexible_type> out_row(3);
      size_t example_id = example_begin;
      std::vector<int> temp_class_index(num_classes);
      std::vector<float> temp_vec_for_binary_preds(2);

      while (example_id < example_end) {
        // Class index vector of {0, 1, 2, ..., num_classes - 1}
        int _i = 0;
        std::generate(temp_class_index.begin(), temp_class_index.end(), [&_i](){ return _i++; });

        // Pointer to the prediction of the first class for this example
        // Advance this pointer gets the prediction of the rest of classes
        auto value_ptr = &(preds[example_id * stride]);
        if (num_classes == 2) {
          temp_vec_for_binary_preds[1] = preds[example_id * stride];
          temp_vec_for_binary_preds[0] = output_type_enum == prediction_type_enum::MARGIN ? 0.0 : 1.0 - (temp_vec_for_binary_preds[1]);
          value_ptr = &(temp_vec_for_binary_preds[0]);
        }

        // Partial sort the tempo_class_index to get topk
        std::partial_sort(temp_class_index.begin(), temp_class_index.begin() + topk, temp_class_index.end(),
                          [&](int i, int j) { return value_ptr[i] > value_ptr[j]; });

        // Write each topk to output
        for (size_t pos = 0; pos < topk; ++pos) {
          size_t idx = temp_class_index[pos];
          auto value = *(value_ptr + idx);
          out_row[0] = example_id;
          out_row[1] = target_indexer->map_index_to_value(idx);
          out_row[2] = output_type_enum == prediction_type_enum::RANK ? pos : value;
          *out_iter++ = out_row;
        }

        // Advance to next example
        ++example_id;
      }
    });
  sf.close();
  DASSERT_EQ(sf.size(), num_examples * topk);
  return sf;
}

/**
 * Trim the model by removing states allocated for training only
 */
void trim_boost_learner(std::shared_ptr<::xgboost::learner::BoostLearner>& booster) {
  std::string tmp_file = get_temp_name();
  bool save_with_pbuffer = false;
  // save model to tmp file
  booster->SaveModel(tmp_file.c_str(), save_with_pbuffer);
  // load model from tmp file
  auto trimmed_model = new ::xgboost::learner::BoostLearner();
  trimmed_model->LoadModel(tmp_file.c_str());
  booster.reset(trimmed_model);
}

/**************************************************************************/
/*                                                                        */
/*                             xgboost_model                              */
/*                                                                        */
/**************************************************************************/

xgboost_model::xgboost_model() {
  booster_ = std::make_shared<BoostLearner>();
}

/**
 * create ml_data to iterator object
 */
void xgboost_model::model_specific_init(const ml_data& data,
                                        const ml_data& valid_data) {
  this->ml_data_ = data;
  this->validation_ml_data_ = valid_data;
}

flexible_type convert_vec_string(const std::vector<std::string> &src) {
  std::vector<flexible_type> vec;
  for( auto &s: src ){
    vec.push_back( s );
  }
  flexible_type hist = vec;
  return hist;
}

bool xgboost_model::is_random_forest() {
  return boost::starts_with(this->name(), "random_forest");
}

void xgboost_model::init_options(const std::map<std::string,flexible_type>& _opts) {
  if (_opts.count("_storage_mode")) {
    int mode = _opts.at("_storage_mode");
    this->_set_storage_mode(static_cast<storage_mode_enum>(mode));
  } else if (_opts.count("_internal_opts")) {
    flex_dict internal_opts = _opts.at("_internal_opts");
    for (auto& kv: internal_opts) {
      std::string key = kv.first;
      std::string value = kv.second;
      logstream(LOG_INFO) << "Set internal learner option: "
                          << key << "=" << value << std::endl;
      booster_->SetParam(key.c_str(), value.c_str());
    }
  }
  if (_opts.count("_num_batches")) {
    int num_batches = _opts.at("_num_batches");
    this->_set_num_batches(num_batches);
  }
  if (_opts.count("metric")) {
    auto parsed_metrics = parse_tracking_metric(_opts.at("metric"), this->tracking_metrics, this->is_classifier());
    this->set_tracking_metric(parsed_metrics);
  }
}

size_t xgboost_model::num_classes() {
  size_t num_classes = 0;
  if (is_classifier()) {
    num_classes = variant_get_value<size_t>(state.at("num_classes"));
  }
  return num_classes;
}

void xgboost_model::_set_storage_mode(storage_mode_enum mode) {
  logstream(LOG_INFO) << "Set storage mode to " << (int)mode << std::endl;
  storage_mode_ = mode;
}

void xgboost_model::_set_num_batches(size_t num_batches) {
  logstream(LOG_INFO) << "Set number of batches to " << num_batches << std::endl;
  num_batches_ = num_batches;
}

std::pair<std::shared_ptr<DMatrixMLData>, std::shared_ptr<DMatrixMLData>> xgboost_model::_init_data() {
  // Class weights
  flexible_type class_weights = flex_undefined();
  if (is_classifier()) {
    class_weights = get_class_weights_from_options(options, this->ml_mdata);
    state["class_weights"] =  to_variant(class_weights);
  }
  // Validation size
  state["num_validation_examples"] = validation_ml_data_.size();

  // Set training data
  std::shared_ptr<DMatrixMLData> ptrain = std::make_shared<DMatrixMLData>(ml_data_, class_weights, storage_mode_, num_batches_);
  // Set validation data
  std::shared_ptr<DMatrixMLData> pvalid;
  if (validation_ml_data_.size() > 0) {
    pvalid = std::make_shared<DMatrixMLData>(this->validation_ml_data_,
                                             class_weights,
                                             storage_mode_,
                                             num_batches_);
  }
  return {ptrain, pvalid};
}

void xgboost_model::_init_learner(std::shared_ptr<DMatrixMLData> ptrain,
                                  std::shared_ptr<DMatrixMLData> pvalid,
                                  bool restore_from_checkpoint,
                                  std::string checkpoint_restore_path) {
  this->configure();
  if (pvalid != nullptr) {
    booster_->SetCacheData({ptrain.get(), pvalid.get()});
  } else {
    booster_->SetCacheData({ptrain.get()});
  }
  // Subclass configurations
  if (ptrain->use_extern_memory_) {
    booster_->SetParam("updater", "grow_histmaker,prune");
  }
  if (!restore_from_checkpoint) {
    booster_->InitModel();
  } else {
    _restore_from_checkpoint(checkpoint_restore_path);
  }
  booster_->CheckInit(ptrain.get());
}

table_printer xgboost_model::_init_progress_printer(bool has_validation_data) {
  // Prepare the progress printing table
  size_t default_column_width = 8;
  size_t metric_column_width = 6;
  std::vector<std::pair<std::string, size_t>> progress_header{
    {"Iteration", default_column_width},
    {"Elapsed Time", default_column_width}
  };
  for (const std::string& metric : tracking_metrics) {
    std::string metric_display_name = get_metric_display_name(metric);
    progress_header.emplace_back("Training " + metric_display_name,
                                 metric_column_width);
    if (has_validation_data) {
      progress_header.emplace_back("Validation " + metric_display_name,
                                   metric_column_width);
    }
  }
  table_printer printer(progress_header);
  return printer;
}

/**
 * Helper datastructure to keep track of training and validation metrics.
 */
class metric_tracker {
 public:
  metric_tracker(xgboost_model& model) {
    metric_names = model.get_tracking_metrics();
    for (auto& m : metric_names) {
      evaluators.push_back(get_fast_evaluator(m, model.num_classes()));
    }
  }
  /// Return a row to print in the progress table
  /// {iteration, time, training-metric1, training-metric2, ... , validation_metric1, ...}
  std::vector<std::string> make_progress_table_row(size_t iter, double time) {
    std::vector<std::string> ret{std::to_string(iter + 1), std::to_string(time)};
    for(size_t i = 0; i < metric_names.size(); ++i) {
      auto& m = metric_names[i];
      ret.push_back(std::to_string(training_metrics.at({m, iter})));
      if (validation_metrics.count({m, iter})) {
        ret.push_back(std::to_string(validation_metrics.at({m, iter})));
      }
    }
    return ret;
  }
  /// Add training metrics for current iteration to record
  void track_training(size_t iteration, std::vector<float>& metrics) {
    for (size_t i = 0; i < metric_names.size(); ++i) {
      const auto& m = metric_names[i];
      auto value = metrics[i];
      training_metrics[{m, iteration}] = value;
    }
  }
  /// Add validation metrics for current iteration to record
  void track_validation(size_t iteration, std::vector<float>& metrics) {
    for (size_t i = 0; i < metric_names.size(); ++i) {
      const auto& m = metric_names[i];
      auto value = metrics[i];
      validation_metrics[{m, iteration}] = value;
    }
  }
  /// Return the underlying evaluators
  std::vector<xgboost_evalptr>& get_evaluators() { return evaluators; }
  /// Return training metrics for iteration "iter"
  std::vector<float> get_training_metrics(size_t iter) const {
    std::vector<float> ret;
    for (size_t i = 0; i < metric_names.size(); ++i) {
      auto& m = metric_names[i];
      ret.push_back(training_metrics.at({m, iter}));
    }
    return ret;
  }
  /// Return validation metrics for iteration "iter", or empty if no validation set
  std::vector<float> get_validation_metrics(size_t iter) const {
    std::vector<float> ret;
    if (validation_metrics.size() > 0) {
      for (size_t i = 0; i < metric_names.size(); ++i) {
        auto& m = metric_names[i];
        ret.push_back(validation_metrics.at({m, iter}));
      }
    }
    return ret;
  }
 private:
  std::vector<xgboost_evalptr> evaluators;
  // store tracking metric value to map[metric_name, iteration]
  std::map<std::pair<std::string, size_t>, float> training_metrics;
  std::map<std::pair<std::string, size_t>, float> validation_metrics;
  std::vector<std::string> metric_names;
}; // end of metric_tracker


/**
 * Shared training code for all xgboost models.
 */
void xgboost_model::train(void) {
  size_t row_limit = std::numeric_limits<bst_uint>::max();
  if (ml_data_.num_rows() > row_limit) {
    log_and_throw(std::string("Tree models cannot be trained on more than ") + std::to_string(row_limit) +
                  " rows. Please reduce the data size or use distributed training.");
  }

  // Restore from checkpoint
  bool restore_from_checkpoint = false;
  std::string checkpoint_restore_path;
  if (options.is_option("resume_from_checkpoint") && (options.value("resume_from_checkpoint") != FLEX_UNDEFINED)) {
    checkpoint_restore_path = (std::string)(options.value("resume_from_checkpoint"));
    std::string sanitized_path = sanitize_url(checkpoint_restore_path);
    this->options.set_option("resume_from_checkpoint", sanitized_path);
    logprogress_stream << "Resuming from checkpoint at " << sanitized_path << std::endl;
    restore_from_checkpoint = true;
  }

  // Checkpoint path
  std::string model_checkpoint_path;
  if (options.is_option("model_checkpoint_path")
      && (options.value("model_checkpoint_path") != FLEX_UNDEFINED)) {
    model_checkpoint_path = (std::string)options.value("model_checkpoint_path");
    this->options.set_option("model_checkpoint_path", sanitize_url(model_checkpoint_path));
  }

  /** Prepare for training:
   * - Initialize training and validation data structure: ptrain, pvalid
   * - Initialize boost learner
   * - Initialize the progress printer
   * - Initialize the evaluator
   * - Initialize early stopping
   **/
  // Data
  std::shared_ptr<DMatrixMLData> ptrain, pvalid;
  std::tie(ptrain, pvalid) = this->_init_data();
  this->_init_learner(ptrain, pvalid,
                      restore_from_checkpoint, checkpoint_restore_path);
  bool has_validation_data = pvalid != nullptr;
  // Progress printer
  table_printer printer = this->_init_progress_printer(has_validation_data);
  std::shared_ptr<unity_sframe> progress_table = std::make_shared<unity_sframe>();
  if (restore_from_checkpoint) {
    progress_table = variant_get_value<std::shared_ptr<unity_sframe>>(state["progress"]);
  }
  // Metric Tracker
  metric_tracker tracker(*this);
  // Early Stopper
  std::shared_ptr<early_stopping_checkpointer> early_stopper;
  size_t early_stopping_rounds = _get_early_stopping_rounds(has_validation_data);
  if (early_stopping_rounds > 0) {
    // Use the last tracking metric for early stopping
    std::string early_stop_metric = tracking_metrics.back();
    bool tracking_max_score = false;
    if (early_stop_metric == "auc" || early_stop_metric == "accuracy") {
      tracking_max_score = true;
    }
    // To return the best model at from t to t + delta, we need to keep delta+1 models.
    size_t max_models_to_keep = early_stopping_rounds + 1;
    early_stopper = std::make_shared<early_stopping_checkpointer>(max_models_to_keep, tracking_max_score, early_stopping_rounds);
  }

  // Main Train Loop
  turi::timer timer;
  timer.start();
  size_t max_iterations = options.is_option("max_iterations") ?  size_t(options.value("max_iterations")) : 1;
  size_t iter = 0;
  if (restore_from_checkpoint) {
    iter = (int)(variant_get_value<flexible_type>(state["num_trees"]));
    if (iter >= max_iterations) {
      logprogress_stream << "Resumed training from checkpoint at iteration " << iter
                         << " which is greater than or equal to max_iterations " << max_iterations
                         << std::endl;
      return;
    }
  }
  printer.print_header();
  while (iter < max_iterations) {
    if (cppipc::must_cancel()) {
      log_and_throw("Canceled by user");
    }
    // Update
    if (this->is_random_forest()) {
      booster_->UpdateOneIterKeepGpair(iter, *ptrain);
    } else {
      booster_->UpdateOneIter(iter, *ptrain);
    }
    // Predict
    std::vector<float> preds;
    bool output_margin = false;
    double rf_running_rescale_constant = 1.0 / (iter+1);
    this->xgboost_predict(*ptrain, output_margin, preds, rf_running_rescale_constant);
    // Evaluate
    std::vector<float> metrics = fast_evaluate(preds, ptrain->info, tracker.get_evaluators());
    tracker.track_training(iter, metrics);
    if (has_validation_data) {
      std::vector<float> valid_preds;
      this->xgboost_predict(*pvalid, output_margin, valid_preds, rf_running_rescale_constant);
      metrics = fast_evaluate(valid_preds, pvalid->info, tracker.get_evaluators());
      tracker.track_validation(iter, metrics);
    }
    // Print
    std::vector<std::string> progress_row = tracker.make_progress_table_row(iter, timer.current_time());
    printer.print_progress_row_strs(iter + 1, progress_row);
    // Check for early stopping
    if (early_stopper != nullptr) {
      early_stopper->add(iter, booster_, tracker.get_validation_metrics(iter).back());
      if (early_stopper->need_stop(iter)) {
        booster_ = early_stopper->get_best_model();
        break;
      }
    }

    // Checkpoint model
    if (!(model_checkpoint_path.empty()) &&
        (int)options.value("model_checkpoint_interval") != 0 &&
        (iter + 1) % (int)options.value("model_checkpoint_interval") == 0) {

      namespace fs = boost::filesystem;
      fs::path checkpoint_path(model_checkpoint_path);
      checkpoint_path /= "model_checkpoint_" + std::to_string(iter+1);
      // Append progress tables
      if (progress_table->size() == 0) {
        progress_table->construct_from_sframe(printer.get_tracked_table());
      } else {
        auto new_progress_table = std::make_shared<unity_sframe>();
        new_progress_table->construct_from_sframe(printer.get_tracked_table());
        progress_table = std::dynamic_pointer_cast<unity_sframe>(progress_table->append(new_progress_table));
      }
      _save_training_state(iter,
                           tracker.get_training_metrics(iter),
                           tracker.get_validation_metrics(iter),
                           progress_table,
                           timer.current_time());
      _checkpoint(checkpoint_path.string());
    }

    ++iter;
  }
  printer.print_footer();
  // Append progress tables
  if (progress_table->size() == 0) {
    progress_table->construct_from_sframe(printer.get_tracked_table());
  } else {
    auto new_progress_table = std::make_shared<unity_sframe>();
    new_progress_table->construct_from_sframe(printer.get_tracked_table());
    progress_table = std::dynamic_pointer_cast<unity_sframe>(progress_table->append(new_progress_table));
  }

  size_t final_iter = max_iterations-1;
  if (iter < max_iterations) {
    final_iter = early_stopper->get_best_iter();;
    logprogress_stream << "Early stop triggered. Returning the best model at iteration: "
                       << (1+final_iter) << std::endl;
  }
  // Save training info to model state
  _save_training_state(final_iter,
                       tracker.get_training_metrics(final_iter),
                       tracker.get_validation_metrics(final_iter),
                       progress_table,
                       timer.current_time());
  // free booster memory allocated for training only
  trim_boost_learner(booster_);
}

enum{
    XGBOOST_WITH_STATS = 1, //output also gain and cover metrics per node
    XGBOOST_JSON_FORMAT = 2 //use Json format for output
};

/**
 * Save the training state as model metadata
 */
void xgboost_model::_save_training_state(size_t iteration,
                                         const std::vector<float>& training_metrics,
                                         const std::vector<float>& validation_metrics,
                                         std::shared_ptr<unity_sframe> progress_table,
                                         double training_time) {
  // Store progress table.
  state["progress"] = to_variant(progress_table);
  // Store evaluation metrics
  std::map<std::string, flexible_type> info;
  info["training_time"] = training_time;

  for (size_t i = 0; i < tracking_metrics.size(); ++i) {
    auto metric = tracking_metrics[i];
    info["training_" + metric] = training_metrics[i];
    if (validation_metrics.size() > 0) {
      info["validation_" + metric] = validation_metrics[i];
    }
  }
  // Store trees
  utils::FeatMap fmap;
  MakeFeatMap(fmap, this->ml_mdata);
  std::vector<flexible_type> trees_json = convert_vec_string(booster_->DumpModel(fmap, XGBOOST_JSON_FORMAT | XGBOOST_WITH_STATS));
  info["trees_json"] = trees_json;
  info["num_trees"] = trees_json.size();
  add_or_update_state(flexmap_to_varmap(info));
}

/**
 * Make predictions using a trained regression model.
 */
std::shared_ptr< sarray<flexible_type> > xgboost_model::predict_impl(
    const DMatrix& dmat,
    const std::string& output_type) {

  std::vector<float> preds;

  // Classification
  if (this->num_classes() > 2) {
    if ((output_type == "margin") || (output_type == "probability")) {
      std::stringstream ss;
      ss << "Output type '" << output_type
         << "' is only supported for binary classification."
         << " For multi-class classification, use predict_topk() instead."
         << std::endl;
      log_and_throw(ss.str());
    }
  }
  this->xgboost_predict(dmat, output_type=="margin", preds);
  return transform_prediction(preds,
                              prediction_type_enum_from_name(output_type),
                              this->num_classes(),
                              this->ml_mdata);
}


std::shared_ptr< sarray<flexible_type> > xgboost_model::predict(
    const ml_data& test_data,
    const std::string& output_type) {
  DMatrixMLData dmat(test_data);
  return predict_impl(dmat, output_type);
}

gl_sarray xgboost_model::fast_predict(
    const std::vector<flexible_type>& test_data,
    const std::string& missing_value_action,
    const std::string& output_type) {
  auto na_enum = get_missing_value_enum_from_string(missing_value_action);
  DMatrixSimple dmat = make_simple_dmatrix(test_data, this->ml_mdata, na_enum);
  auto sa = predict_impl(dmat, output_type);
  auto unity_sa = std::make_shared<unity_sarray>();
  unity_sa->construct_from_sarray(sa);
  return gl_sarray(unity_sa);
}

gl_sframe xgboost_model::fast_predict_topk(
    const std::vector<flexible_type>& test_data,
    const std::string& missing_value_action,
    const std::string& output_type,
    const size_t topk) {
  auto na_enum = get_missing_value_enum_from_string(missing_value_action);
  DMatrixSimple dmat = make_simple_dmatrix(test_data, this->ml_mdata, na_enum);
  auto sf = predict_topk_impl(dmat, output_type, topk);
  auto unity_sf = std::make_shared<unity_sframe>();
  unity_sf->construct_from_sframe(sf);
  return gl_sframe(unity_sf);

}

sframe xgboost_model::predict_topk(
    const ml_data& test_data,
    const std::string& output_type,
    const size_t topk) {
  DMatrixMLData dmat(test_data);
  return predict_topk_impl(dmat, output_type, topk);
}


/**
 * Helper function for making predictions using the internal xgboost
 */
void xgboost_model::xgboost_predict(const DMatrix& dmat,
                                    bool output_margin,
                                    std::vector<float>& out_preds,
                                    double rf_running_rescale_constant) {
  // Rescale random forest predictions. Boosted trees are grown with
  // respect to the residuals given previous tress, where as for random
  // forests we grow each tree with respect to the original targets.
  // Thus we need an average prediction across trees, and we do this by
  // rescaling the predictions by 1/max_iterations below.
  double rescale_constant = 1.0;
  if (this->is_random_forest()) {
    if (rf_running_rescale_constant == 0.0) {
      size_t max_iterations = options.value("max_iterations").get<flex_int>();
      rescale_constant = 1.0 / max_iterations;
    } else {
      rescale_constant = rf_running_rescale_constant;
    }
  }
  size_t ntree_limit = 0;
  bool pred_leaf = false;
  booster_->Predict(dmat, output_margin, &out_preds, ntree_limit, pred_leaf, rescale_constant);

  // Correct the margin. Multclass margin should be relative to zero.
  // We set class 0's margin to zero and, minus it from the margin of other classes.
  size_t num_classes = this->num_classes();
  if (output_margin && num_classes > 2) {
    double base_score = 0.0;
    for (size_t i = 0; i < out_preds.size(); ++i) {
      if (i % num_classes == 0) {
        base_score = out_preds[i];
        out_preds[i] = 0.0;
      } else {
        out_preds[i] -= base_score;
      }
    }
  }
}

/**
 * Fast predict_topk
 */
sframe xgboost_model::predict_topk_impl(
    const DMatrix& dmat,
    const std::string& output_type,
    const size_t topk) {

  // Must be a classifier.
  DASSERT_TRUE(this->is_classifier());

  size_t num_classes = this->num_classes();

  if (topk > num_classes) {
    std::stringstream ss;
    ss << "The training data contained " << num_classes << " classes."
       << " The parameter 'k' must be less than or equal to the number of "
       << "classes in the training data." << std::endl;
    log_and_throw(ss.str());
  }

  std::vector<float> preds;
  this->xgboost_predict(dmat, output_type=="margin", preds);
  return transform_prediction_topk(preds,
                                   output_type,
                                   topk,
                                   num_classes,
                                   this->ml_mdata);
}

/**
 * First make predictions and then evaluate.
 */
std::map<std::string, variant_type> xgboost_model::evaluate(
    const ml_data& test_data,
    const std::string& evaluation_type,
    bool with_prediction) {
  // XGboost data format conversion.
  DMatrixMLData dmat(test_data);
  return evaluate_impl(dmat, evaluation_type);
}

/**
 * First make predictions and then evaluate.
 *
 * TODO: This code is *very* similar to the code in supervised_learning.cpp.
 * the fix would be to implement a predict_single_example interface that
 * works accross both models.
 */
std::map<std::string, variant_type> xgboost_model::evaluate_impl(
    const DMatrixMLData& dmat,
    const std::string& evaluation_type) {
  // Classifier specific metrics pre-computations.
  std::map<size_t, flexible_type> index_map;
  std::unordered_map<flexible_type, size_t> identity_map;
  size_t num_classes = this->num_classes();
  if (is_classifier()) {
    for (size_t i = 0; i < dmat.num_classes(); i++) {
      index_map[i] = this->ml_mdata->target_indexer()->map_index_to_value(i);
      identity_map[i] = i;
    }
  }
  std::map<std::string, variant_type> kwargs {
    {"average", to_variant(std::string("default"))},
    {"num_classes", to_variant(num_classes)},
    {"inv_index_map", to_variant(index_map)},
    {"binary", to_variant(false)},
    {"index_map", to_variant(identity_map)},
        };

  // Setup metric computation.
  std::vector<std::string> evaluator_names;
  std::vector<evalptr> evaluators;
  size_t n_threads = thread::cpu_count();

  // Compute a specific metric or all metrics ["auto"]
  std::vector<std::string> metrics_computed;
  if (evaluation_type == std::string("auto")) {
    metrics_computed = metrics;
    DASSERT_TRUE(metrics_computed.size() > 0);
  } else if (evaluation_type == std::string("train")) {
    metrics_computed = tracking_metrics;
    DASSERT_TRUE(metrics_computed.size() > 0);
  } else {
    metrics_computed.push_back(evaluation_type);
  }

  // Get the evaluator metrics.
  bool contains_prob_evaluator = false;
  for (const auto& m: metrics_computed){
    // Remove metrics that can't be computed.
    auto e = evaluation::get_evaluator_metric(m, kwargs);
    evaluators.push_back(e);
    evaluator_names.push_back(m);
    // If a prob-evaluator is needed, then we use prediction probabilities.
    if (! contains_prob_evaluator) {
      contains_prob_evaluator = e->is_prob_evaluator();
    }
  }
  DASSERT_TRUE(evaluators.size() > 0);
  DASSERT_TRUE(metrics_computed.size() > 0);

  // Init the evaluators
  for(size_t i=0; i < evaluators.size(); i++){
    evaluators[i]->init(n_threads);
  }

  // Write target to an SArray.
  flex_type_enum type = flex_type_enum::FLOAT;
  if (is_classifier()) {
    type = flex_type_enum::INTEGER;
  }
  gl_sarray_writer writer(type);
  for (const auto& t: dmat.info.labels) {
    writer.write(flexible_type(t), 0);
  }
  gl_sarray targets = writer.close();
  gl_sframe eval_sf {{"targets", targets}};

  // Make predictions and save them to an SFrame.
  auto unity_sa = std::make_shared<unity_sarray>();
  if (is_classifier()) {
    unity_sa->construct_from_sarray(predict_impl(dmat, "class_index"));
    eval_sf.add_column(gl_sarray(unity_sa), "preds");
    if (contains_prob_evaluator) {
      unity_sa->construct_from_sarray(predict_impl(dmat, "probability_vector"));
      eval_sf.add_column(gl_sarray(unity_sa), "prob_preds");
    }
  } else {
    unity_sa->construct_from_sarray(predict_impl(dmat));
    eval_sf.add_column(gl_sarray(unity_sa), "preds");
  }

  // Evaluate!
  size_t src_size = eval_sf.size();
  in_parallel([&](size_t thread_idx, size_t num_threads) {
      size_t start_idx = src_size * thread_idx / num_threads;
      size_t end_idx = src_size * (thread_idx + 1) / num_threads;

      for (const auto& v: eval_sf.range_iterator(start_idx, end_idx)) {

        for(size_t i=0; i < evaluators.size(); i++){
          if (evaluators[i]->is_prob_evaluator()) {
            evaluators[i]->register_example(v[0], v[2], thread_idx);
          } else {
            evaluators[i]->register_example(v[0], v[1], thread_idx);
          }
        }
      }  // End range iterator.
    }); // End parallel evaluation

  // Get results
  std::map<std::string, variant_type> results;
  for(size_t i=0; i < evaluators.size(); i++){
    results[evaluator_names[i]] = evaluators[i]->get_metric();
  }
  return results;
}

std::shared_ptr<sarray<flexible_type>> xgboost_model::extract_features(
    const sframe& test_data,
    ml_missing_value_action missing_value_action) {
  std::vector<float> out;
  ml_data data = construct_ml_data_using_current_metadata(
      test_data, missing_value_action);
  DMatrixMLData dmat(data);
  bool output_margin = false; // doesn't get used if pred_leaf is true
  size_t num_trees = 0; // doesn't get used if pred_leaf is true
  bool pred_leaf = true;
  booster_->Predict(dmat, output_margin, &out, num_trees, pred_leaf);

  num_trees = options.is_option("max_iterations") ?
      options.value("max_iterations").get<flex_int>() : 1;

  size_t stride = 1;
  if (is_classifier()) {
    size_t num_classes = this->num_classes();
    if (num_classes > 2)
      stride = num_classes;
  }

  DASSERT_EQ(out.size(), data.num_rows() * num_trees * stride);
  std::shared_ptr<sarray<flexible_type>> ret (new sarray<flexible_type>);

  ret->open_for_write(1);
  ret->set_type(flex_type_enum::VECTOR);
  auto writer = ret->get_output_iterator(0);

  flex_vec buffer(num_trees * stride);
  size_t iter = 0;
  for (size_t i = 0; i < data.num_rows(); ++i) {
    for (size_t j = 0; j < num_trees * stride; ++j) {
      buffer[j] = out[iter++];
    }
    *writer++ = buffer;
  }
  ret->close();
  return ret;
}

/**
 * Utility to convert the metadata into a feature map.
 */
utils::FeatMap get_index_map_with_escaping(
    const std::shared_ptr<ml_metadata>& metadata) {

  // Create a feature map to be sent to XGboost.
  const char* format_str = "{%zd}\0";
  utils::FeatMap _index_fmap;

  char feature_name[256];
  auto to_index_info = [&](char* buf, size_t col, size_t feature_index) {
    size_t index = (metadata->global_index_offset(col) + feature_index);
    TURI_ATTRIBUTE_UNUSED_NDEBUG int n = snprintf(buf, 256, format_str, index);
    DASSERT_GT(n, 0);
    DASSERT_LT(n, 256);
    DASSERT_EQ(buf[n], '\0');
    return index;
  };

  for (size_t col = 0; col < metadata->num_columns(); ++col) {
    switch(metadata->column_mode(col)) {
      case ml_column_mode::NUMERIC: {
        const char* xg_type_code = (metadata->column_type(col) ==
                                    flex_type_enum::INTEGER) ? "int" : "q";
        size_t index = to_index_info(feature_name, col, 0);
        _index_fmap.PushBack(index, feature_name, xg_type_code);
        break;
      }
      case ml_column_mode::NUMERIC_VECTOR:
      case ml_column_mode::NUMERIC_ND_VECTOR: {
        for (size_t offset = 0; offset < metadata->index_size(col); ++offset) {
          size_t index = to_index_info(feature_name, col, offset);
          _index_fmap.PushBack(index, feature_name, "q");
        }
        break;
      }
      case ml_column_mode::CATEGORICAL_VECTOR:
      case ml_column_mode::CATEGORICAL: {
        for (size_t offset = 0; offset < metadata->index_size(col); ++offset) {
          size_t index = to_index_info(feature_name, col, offset);
          _index_fmap.PushBack(index, feature_name, "i");
        }
        break;
      }
      case ml_column_mode::DICTIONARY: {
        for (size_t offset = 0; offset < metadata->index_size(col); ++offset) {
          size_t index = to_index_info(feature_name, col, offset);
          _index_fmap.PushBack(index, feature_name, "q");
        }
        break;
      }
      default: DASSERT_TRUE(false);
    }
  }
  return _index_fmap;
}

/**
 * Get the tree from XGboost (in text format).
 */
flexible_type xgboost_model::get_trees() {
  auto metadata = this->ml_mdata;

  // Escaping is needed to reliably do indexing.
  utils::FeatMap _index_fmap = get_index_map_with_escaping(metadata);

  // Dump the model from the booster (with the added information).
  std::vector<std::string> trees = booster_->DumpModel(_index_fmap, 2);
  return convert_vec_string(trees);
}

/**
 * Get a single tree from XGboost.
 */
flexible_type xgboost_model::get_tree(size_t tree_id) {
  flex_list trees = get_trees().get<flex_list>();

  // Check tree size.
  size_t max_trees = trees.size();
  if (tree_id >= trees.size()) {
    std::stringstream ss;
    ss << "Cannot retrive 'tree_id'= " << tree_id
       << ". This model has a maximum of " << max_trees << "."
       << std::endl;
    log_and_throw(ss.str());
  }

  // Return the tree.
  return trees[tree_id];
}


/**
 * Get importance score of features.
 *
 * The importance is measured by the count of feature across all trees.
 *
 * The returned SFrame has three columns: name, index and count,
 * and is sorted by "count":
 *  "name" is the feature column name, e.g. "Gender"
 *  "index" is the "value" of categorical column,
 *     e.g. "M" or "F" for "Gender". For numerical column, index is "None".
 *  "count" is the total ocurrence of the feature[index] in all trees.
 *
 * Implementation:
 * 1. Dump xgboost tree models using the feature index in ml data.
 * 2. Aggregate the count of each feature index over all trees.
 * 3. Write out the return SFrame with schema {"name", "index", "count"}.
 */
gl_sframe xgboost_model::get_feature_importance() {

  auto metadata = this->ml_mdata;

  // Make a custom feature map to make sure it's correct.
  const char* format_str = "{%zd}\0";

  utils::FeatMap _index_fmap = get_index_map_with_escaping(metadata);

  std::vector<std::string> trees = booster_->DumpModel(_index_fmap, 0);
  std::vector<size_t> counts(this->ml_mdata->num_dimensions(), 0);

  for (const std::string& tree : trees) {
    const char* s = tree.c_str();

    for(size_t pos = 0; pos < tree.size(); ++pos) {

      if(s[pos] == '{') {

        size_t idx = size_t(-1);
        int ret = sscanf(s + pos, format_str, &idx);
        if(ret == EOF) continue;
        DASSERT_EQ(ret, 1);  // Number of arguments filled out.

        do {
          ++pos;
          DASSERT_LT(pos, tree.size());
        } while(s[pos] != '}');

        DASSERT_TRUE(idx < counts.size());
        counts[idx] += 1;
      }
    }
  }

  sframe coeff_count_sf;
  coeff_count_sf.open_for_write({"name", "index", "count"},
                                {flex_type_enum::STRING, flex_type_enum::STRING,
                                      flex_type_enum::INTEGER},
                                "",
                                1);

  auto out_it = coeff_count_sf.get_output_iterator(0);
  std::vector<flexible_type> out(3);

  size_t pos = 0;
  for(size_t col_index = 0; col_index < metadata->num_columns(); ++col_index) {
    out[0] = metadata->column_name(col_index);

    switch(metadata->column_mode(col_index)) {

      case ml_column_mode::DICTIONARY:
      case ml_column_mode::CATEGORICAL:
      case ml_column_mode::CATEGORICAL_VECTOR:
        {
          for(size_t i = 0; i < metadata->index_size(col_index); ++i) {
            out[1] = metadata->indexer(
                col_index)->map_index_to_value(i).to<flex_string>();
            out[2] = counts[pos];
            *out_it = out;
            ++out_it, ++pos;
          }
          break;
        }
      case ml_column_mode::NUMERIC_VECTOR:
      case ml_column_mode::NUMERIC_ND_VECTOR: {
        for(size_t i = 0; i < metadata->index_size(col_index); ++i) {
          out[1] = std::to_string(i);
          out[2] = counts[pos];
          *out_it = out;
          ++out_it, ++pos;
        }
        break;
      }
      case ml_column_mode::NUMERIC: {
        out[1] = flex_undefined();
        out[2] = counts[pos];
        *out_it = out;
        ++out_it, ++pos;
        break;
      }
      default: ASSERT_TRUE(false);
    }
  }

  DASSERT_EQ(pos, counts.size());
  coeff_count_sf.close();
  return gl_sframe(coeff_count_sf).sort("count", false);
}


std::vector<std::string> xgboost_model::dump(bool with_stats) {
  utils::FeatMap fmap;
  MakeFeatMap(fmap, this->ml_mdata);
  int option = 0; // text format
  option |= with_stats;
  std::vector<std::string> trees = booster_->DumpModel(fmap, option);
  return trees;
}

std::vector<std::string> xgboost_model::dump_json(bool with_stats) {
  utils::FeatMap fmap;
  MakeFeatMap(fmap, this->ml_mdata);
  int option = 2; // json format
  option |= with_stats;
  std::vector<std::string> trees = booster_->DumpModel(fmap, option);
  return trees;
}

void xgboost_model::_save(turi::oarchive& oarc, bool save_booster_prediction_buffer) const {
  // State
  variant_deep_save(state, oarc);
  // Everything else
  oarc << ml_mdata
       << metrics
       << options;
  // XGBoost model
  ArcStreamOutAdapter fo(oarc);
  booster_->SaveModel(fo, save_booster_prediction_buffer);
}

void xgboost_model::save_impl(turi::oarchive& oarc) const {
  // prediction buffer is saved for checkpoint/restore, no here.
  bool save_booster_prediction_buffer = false;
  _save(oarc, save_booster_prediction_buffer);
}

void xgboost_model::load_version(turi::iarchive& iarc, size_t version) {
  ASSERT_MSG(version <= XGBOOST_MODEL_VERSION,
             "This model version cannot be loaded. Please re-save your state.");
  if (version < 9) {
    log_and_throw("Cannot load a model saved using a version prior to GLC-1.7.");
  }

  // State
  variant_deep_load(state, iarc);

  // Everythiing else
  iarc >> ml_mdata
       >> metrics
       >> options;

  // Version 4 starts storing random seed
  if (version < 4){
    options.create_integer_option(
        "random_seed",
        "Seed for row and column subselection",
        flex_undefined(),
        std::numeric_limits<int>::min() + 1,
        std::numeric_limits<int>::max(),
        false);
    state["random_seed"] = to_variant(FLEX_UNDEFINED);
  }

  // Version 5 starts using tracking_metrics
  if (version < 5) {
    tracking_metrics = metrics;
    this->set_default_evaluation_metric();
  }

  // Version 7 starts storing progress as model state.
  if (version < 7) {
    state["progress"] = to_variant(FLEX_UNDEFINED);
  }

  ArcStreamInAdapter fi(iarc);
  // Version 8 starts using single precision xgboost model.
  if (version < 8) {
    booster_->LoadLegacyModel(fi);
  } else {
    booster_->LoadModel(fi);
  }

  // Version 9 renames num_trees option to be max_iterations
  if ((version < 9) && (this->is_random_forest())) {
    // Add max_iterations to the options manager, setting it
    // to the old num_trees value.
    options.create_integer_option(
        "max_iterations",
        "Maximum number of iterations to perform.",
        10,
        1,
        std::numeric_limits<int>::max(),
        false);
    options.set_option("max_iterations", options.value("num_trees"));

    // Make sure it is available to list_fields.
    state["max_iterations"] = options.value("max_iterations");

    // Delete num_trees as an option, but not from the state.
    // This may be useful information for the user.
    options.delete_option("num_trees");
    options.delete_option("step_size");
    state.erase("step_size");

  }
}

std::shared_ptr<coreml::MLModelWrapper> xgboost_model::_export_xgboost_model(bool is_classifier,
      bool is_random_forest,
      const std::map<std::string, flexible_type>& context) {

  flex_list tree_fl = this->get_trees().get<flex_list>();
  std::vector<std::string> trees(tree_fl.begin(), tree_fl.end());

  return export_xgboost_model(ml_mdata, trees, is_classifier, is_random_forest,
                              context);
}

}  // namespace xgboost
}  // namespace supervised
}  // namespace turi
