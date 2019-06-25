/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FACTORIZATION_GENERAL_LINEAR_MODEL_SECOND_ORDER_H_
#define TURI_FACTORIZATION_GENERAL_LINEAR_MODEL_SECOND_ORDER_H_

#include <Eigen/Core>
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <core/util/branch_hints.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <toolkits/factorization/factorization_model.hpp>
#include <toolkits/factorization/factors_to_sframe.hpp>
#include <model_server/lib/extensions/model_base.hpp>
#include <core/util/fast_top_k.hpp>

namespace turi { namespace factorization {

/**  The model factor mode.  This enum determines the particular mode
 *   that the base class operates in.
 */
enum class model_factor_mode {
  factorization_machine,
    matrix_factorization,
    pure_linear_model};

////////////////////////////////////////////////////////////////////////////////

template <model_factor_mode _factor_mode, flex_int _num_factors_if_known>
class factorization_model_impl final : public factorization_model {
public:

  ////////////////////////////////////////////////////////////////////////////////
  // Set up the types and constants governing the behavior of the model.

  static constexpr model_factor_mode factor_mode = _factor_mode;
  static constexpr flex_int num_factors_if_known = _num_factors_if_known;

  typedef Eigen::Matrix<float, 1,              num_factors_if_known, Eigen::RowMajor> factor_type;
  typedef Eigen::Matrix<float, Eigen::Dynamic, num_factors_if_known, Eigen::RowMajor> factor_matrix_type;
  typedef Eigen::Matrix<float, Eigen::Dynamic, 1>                                     vector_type;

  ////////////////////////////////////////////////////////////////////////////////
  // Declare flags governing how the model works.

  size_t _num_factors           = 0;
  inline size_t num_factors() const {
    if(_num_factors == 0)
      DASSERT_TRUE(factor_mode == model_factor_mode::pure_linear_model);

    return _num_factors_if_known == Eigen::Dynamic ? _num_factors : _num_factors_if_known;
  }

  size_t num_factor_dimensions = 0;

  bool enable_intercept_term  = true;
  bool enable_linear_features = true;
  bool nmf_mode               = false;

  ////////////////////////////////////////////////////////////////////////////////
  // Declare model variables.

  volatile double w0 = NAN;
  vector_type w;
  factor_matrix_type V;

  ////////////////////////////////////////////////////////////////////////////////
  // Declare variables for calculating things.

  size_t n_threads = 1;
  size_t max_row_size = 0;

  struct calculate_fx_processing_buffer {
    mutable factor_matrix_type XV;
    mutable factor_type xv_accumulator;
  };

  std::vector<calculate_fx_processing_buffer> buffers;

  /** Sets up the processing buffers.  Called after internal_setup and
   *  after internal_load.
   */
  void setup_buffers() {

    if(num_factors() == 0) {
      DASSERT_TRUE(factor_mode == model_factor_mode::pure_linear_model);
    }

    // Set the number of threads
    n_threads = thread::cpu_count();

    // Set up the intermediate computing buffers
    buffers.resize(n_threads);
    for(calculate_fx_processing_buffer& buffer : buffers) {
      buffer.xv_accumulator.resize(num_factors());
      buffer.XV.resize(max_row_size, num_factors());
    }

    recommend_cache.resize(n_threads);
  }


public:

  /// Clone the current model
  std::shared_ptr<factorization_model> clone() const {
    return std::shared_ptr<factorization_model>(new factorization_model_impl(*this));
  }

  ////////////////////////////////////////////////////////////////////////////////
  //  Functions to initialize and calculate parts of the model.

  /** Set up some of the internal processing constants and buffers,
   * etc.
   */
  void internal_setup(const v2::ml_data& train_data) {
    // Set the number of factors; this is model dependent.
    switch(factor_mode) {
      case model_factor_mode::factorization_machine:
        _num_factors = options.at("num_factors");
        num_factor_dimensions = n_total_dimensions;
        break;
      case model_factor_mode::matrix_factorization:
        _num_factors = options.at("num_factors");
        num_factor_dimensions = index_sizes[0] + index_sizes[1];
        break;
      case model_factor_mode::pure_linear_model:
        _num_factors = 0;
        num_factor_dimensions = 0;
        break;
    }

    nmf_mode               = options.at("nmf");
    enable_linear_features = !nmf_mode;
    enable_intercept_term  = !nmf_mode;

    if(num_factors_if_known != Eigen::Dynamic)
      DASSERT_EQ(num_factors_if_known, _num_factors);

    max_row_size = train_data.max_row_size();

    setup_buffers();

    w.resize(n_total_dimensions);
    V.resize(num_factor_dimensions, num_factors());

    reset_state(options.at("random_seed"), 0.001);
  }

  /** Initialize the model at a random starting point.  Is
   *  deterministic based on the random seed and num_threads.
   */
  void reset_state(size_t random_seed, double sd) GL_HOT {

    // Normalize it -- otherwise, the factors could really blow this
    // up.
    const double V_sd = sd / (1 + std::sqrt(num_factors()));

    size_t num_factor_init_random = index_sizes[0] + index_sizes[1];
    size_t num_factor_init_zero = num_factor_dimensions - num_factor_init_random;

    in_parallel([&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT_FLATTEN) {
        random::seed(hash64(random_seed, thread_idx, num_threads));

        // Compute the w part.
        if(enable_linear_features) {

          size_t start_w_idx = (thread_idx * n_total_dimensions) / num_threads;
          size_t end_w_idx = ((thread_idx + 1) * n_total_dimensions) / num_threads;

          for(size_t i = start_w_idx; i < end_w_idx; ++i)
            w[i] = (sd > 0) ? random::fast_uniform<double>(-sd/2, sd/2) : 0;
        } else {
          w.setZero();
        }

        // Compute the V part
        {
          size_t start_V_idx = (thread_idx * num_factor_init_random) / num_threads;
          size_t end_V_idx = ((thread_idx + 1) * num_factor_init_random) / num_threads;

          for(size_t i = start_V_idx; i < end_V_idx; ++i) {
            for(size_t j = 0; j < num_factors(); ++j) {

              double lb = nmf_mode ? 0 : -V_sd / 2;
              double ub = nmf_mode ? V_sd : V_sd / 2;

              // Now, to promote diversity at the beginning, only have
              // a handful of the factor terms on each particular
              // factor vector be initialized to a larger value than
              // the rest.  On the rest, just downscale the std dev of
              // the starting value by 1000 or so.
              //
              // Here, each latent factor starts off with about 8
              // terms that are large and the rest small.  On
              // experiments with the Amazon dataset (350m
              // observations, num_factors > 100), this gave good
              // starting values and didn't diverge on reset.

              V(i, j) = (V_sd > 0) ? random::fast_uniform<double>(lb, ub) : 0;

              if(random::fast_uniform<size_t>(0, num_factors()) > std::min<size_t>(4ULL, num_factors() / 2))
                V(i, j) /= 1000;
            }
          }
        }

        // Compute the V part
        if(num_factor_init_zero > 0) {
          size_t start_V_idx = num_factor_init_random + (thread_idx * num_factor_init_zero) / num_threads;
          size_t end_V_idx = num_factor_init_random + ((thread_idx + 1) * num_factor_init_zero) / num_threads;

          for(size_t i = start_V_idx; i < end_V_idx; ++i) {
            for(size_t j = 0; j < num_factors(); ++j) {
              V(i, j) = 0;
            }
          }
        }

      });

    w0 = nmf_mode ? 0 : target_mean;
  }

  /** Calculate the linear function value at the given point.
   *
   *  x is the observation vector it the standard ml_data_entry format.
   *  Each entry of x is an ml_data_entry structure containing the
   *  column index, index, and value of each observation point.  See
   *  ml_data_iterator for more information on it.
   *
   *  In the context of the recommender system, x[0] is the info about
   *  the user and x[1] is the info about the item.  x[0].index is the
   *  user's index, and x[1].index is the item's index.  As for all
   *  categorical variables, the value is 1.
   */
  double calculate_fx(size_t thread_idx, const std::vector<v2::ml_data_entry>& x) const GL_HOT_FLATTEN {

    const size_t x_size = x.size();

    // Depending on the model, do the calculation in the most effecient way possible

    switch(factor_mode) {

      ////////////////////////////////////////////////////////////////////////////////
      // Case 1: Factorization Machine

      case model_factor_mode::factorization_machine: {

        factor_matrix_type& XV = buffers[thread_idx].XV;

        DASSERT_GE(size_t(XV.rows()), x_size);

        factor_type& xv_accumulator = buffers[thread_idx].xv_accumulator;
        xv_accumulator.setZero();

        double fx_value = w0;

        size_t idx = 0;

        for(size_t j = 0; j < x_size; ++j) {
          const v2::ml_data_entry& v = x[j];

          // Check if this feature has been seen before; if not, the
          // corresponding factors are assumed to be zero and to have no
          // effect on any of the totals below; thus we just skip them.
          if(__unlikely__(v.index >= index_sizes[v.column_index]))
            continue;

          const size_t global_idx = index_offsets[v.column_index] + v.index;

          double value_shift, value_scale;
          std::tie(value_shift, value_scale) = this->column_shift_scales[global_idx];

          double xv = value_scale * (x[j].value - value_shift);

          XV.row(idx) = xv * V.row(global_idx);
          xv_accumulator += XV.row(idx);

          fx_value += xv * w[global_idx];

          ++idx;
        }

        for(size_t j = 0; j < idx; ++j)
          fx_value += 0.5*(xv_accumulator.dot(XV.row(j)) - XV.row(j).squaredNorm());

        return fx_value;
      }

        ////////////////////////////////////////////////////////////////////////////////
        // Case 2: Matrix Factorization

      case model_factor_mode::matrix_factorization: {

        factor_matrix_type& XV = buffers[thread_idx].XV;

        DASSERT_GE(size_t(XV.rows()), x_size);
        DASSERT_EQ(size_t(XV.cols()), num_factors());

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Step 1: Calculate the first two dimensions

        double fx_value = w0;

        for(size_t j : {0, 1}) {
          const v2::ml_data_entry& v = x[j];

          // The column index is just going to be 0 or 1.
          DASSERT_EQ(v.column_index, j);

          // Check if this feature has been seen before; if not, the
          // corresponding factors are assumed to be zero and to have no
          // effect on any of the totals below; thus we just skip them.
          if(__unlikely__(v.index >= index_sizes[v.column_index])) {

            XV.row(j).setZero();

          } else {

            // Get the global index
            const size_t global_idx = index_offsets[j] + v.index;

            // No column scaling on the first two dimensions under MF model.
            DASSERT_EQ(v.value, 1);

            XV.row(j) = V.row(global_idx);

            // Add in the contribution
            fx_value += w[global_idx];
          }
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        //  Step 2: Pull in the contribution from the product terms.

        fx_value += XV.row(0).dot(XV.row(1));

        ////////////////////////////////////////////////////////////////////////////////
        // Step 3: Calculate the dimensions past the first two.  These
        // can have anything in them.

        for(size_t j = 2; j < x_size; ++j) {
          const v2::ml_data_entry& v = x[j];

          // Check if this feature has been seen before; if not, the
          // corresponding factors are assumed to be zero and to have no
          // effect on any of the totals below; thus we just skip them.
          if(__unlikely__(v.index >= index_sizes[v.column_index]))
            continue;

          // Get the global index
          const size_t global_idx = index_offsets[v.column_index] + v.index;

          // Set the scaling on this column
          double value_shift, value_scale;
          std::tie(value_shift, value_scale) = column_shift_scales[global_idx];

          double xv = value_scale * (v.value - value_shift);

          fx_value += xv * w[global_idx];
        }

        return fx_value;
      }

        ////////////////////////////////////////////////////////////////////////////////
        // Case 3: Linear Model

      case model_factor_mode::pure_linear_model: {

        double fx_value = w0;

        for(size_t j = 0; j < x_size; ++j) {
          const v2::ml_data_entry& v = x[j];

          // Check if this feature has been seen before; if not, the
          // corresponding factors are assumed to be zero and to have no
          // effect on any of the totals below; thus we just skip them.
          if(__unlikely__(v.index >= index_sizes[v.column_index]))
            continue;

          // Get the global index
          const size_t global_idx = index_offsets[v.column_index] + v.index;

          // Set the scaling on this column
          double value_shift, value_scale;
          std::tie(value_shift, value_scale) = column_shift_scales[global_idx];

          double xv = value_scale * (v.value - value_shift);

          fx_value += xv * w[global_idx];
        }

        return fx_value;
      }

      default:
        return 0;
    }
  }

  /** Calculate the linear function value at the given point.
   *
   *  This is an overload of the above function that does not require
   *  the thread_idx parameter.
   */
  double calculate_fx(const std::vector<v2::ml_data_entry>& x) const GL_HOT_FLATTEN {
    size_t thread_idx = thread::thread_id();

    ASSERT_MSG(thread_idx < buffers.size(),
               "Threading set up in nonstandard way; thread_id() larger than cpu_count().");

    return calculate_fx(thread_idx, x);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Saving and loading of the model.

  void get_item_similarity_scores(
      size_t item, std::vector<std::pair<size_t, double> >& sim_scores) const {

    switch(factor_mode) {
      case model_factor_mode::factorization_machine:
      case model_factor_mode::matrix_factorization:
        {
          // Just go through calculating the cosine metric
          if(item >= index_sizes[1]) {
            for(auto& p : sim_scores) {
              p.second = 0;
            }
            return;
          }

          auto base_row = V.row(index_offsets[1] + item);

          float it_r = base_row.squaredNorm();

          for(auto& p : sim_scores) {
            if(p.first >= index_sizes[1]) {
              p.second = 0;
              continue;
            }

            size_t idx = index_offsets[1] + p.first;
            auto item_row = V.row(idx);
            p.second = item_row.dot(base_row) / std::sqrt(it_r * item_row.squaredNorm());
          }

          break;
        }
      case model_factor_mode::pure_linear_model:
        {
          // Do nothing here.
          return;
        }
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Saving and loading of the model.

  std::map<std::string, variant_type> get_serialization_parameters() const {

    std::map<std::string, variant_type> save_parameters;

    std::string factor_mode_str;

    switch(factor_mode) {
      case model_factor_mode::factorization_machine:
        factor_mode_str = "factorization_machine";
        break;
      case model_factor_mode::matrix_factorization:
        factor_mode_str = "matrix_factorization";
        break;
      case model_factor_mode::pure_linear_model:
        factor_mode_str = "pure_linear_model";
        break;
    }

    save_parameters["factor_mode"] = to_variant(factor_mode_str);

    size_t __num_factors_if_known = num_factors_if_known;

    save_parameters["num_factors_if_known"] = to_variant(__num_factors_if_known);

    return save_parameters;
  }


  ////////////////////////////////////////////////////////////////////////////////
  // Saving and loading of the model.

  size_t get_version() const { return 1; }

  /**  Save routine.
   */
  void save_impl(turi::oarchive& oarc) const {

    std::map<std::string, variant_type> terms;

    // Dump the model parameters.
    terms["_num_factors"]             =  to_variant(num_factors());
    terms["num_factor_dimensions"]    =  to_variant(num_factor_dimensions);
    terms["enable_intercept_term"]  =  to_variant(enable_intercept_term);
    terms["enable_linear_features"] =  to_variant(enable_linear_features);
    terms["nmf_mode"]               =  to_variant(nmf_mode);
    terms["max_row_size"]           =  to_variant(max_row_size);

    variant_deep_save(to_variant(terms), oarc);

    // Now dump out the other things.
    double _w0 = w0;
    oarc << _w0 << w << V;
  }

  /** Load routine.
   */
  void load_version(turi::iarchive& iarc, size_t version) {
    DASSERT_EQ(version, 1);

    variant_type terms_v;

    variant_deep_load(terms_v, iarc);

    auto terms = variant_get_value<std::map<std::string, variant_type> >(terms_v);

#define __EXTRACT(varname)                                              \
    varname = variant_get_value<decltype(varname)>(terms.at(#varname));

    __EXTRACT(_num_factors);
    __EXTRACT(num_factor_dimensions);
    __EXTRACT(enable_intercept_term);
    __EXTRACT(enable_linear_features);
    __EXTRACT(nmf_mode);
    __EXTRACT(max_row_size);

#undef __EXTRACT

    // Now dump out the other things.
    double _w0;
    iarc >> _w0 >> w >> V;
    w0 = _w0;

    setup_buffers();
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Retrieve the coefficients of the model.

  std::map<std::string, variant_type> get_coefficients() const {

    std::map<std::string, variant_type> ret;

    ////////////////////////////////////////
    // Add in the intercept

    if(enable_intercept_term)
      ret["intercept"] = double(w0);

    ////////////////////////////////////////
    // Add in the user and item terms
    {
      bool include_V_term = true;
      bool include_w_term = enable_linear_features;

      switch(factor_mode) {
        case model_factor_mode::factorization_machine:
        case model_factor_mode::matrix_factorization:
          include_V_term = true;
          break;
        case model_factor_mode::pure_linear_model:
          include_V_term = false;
          break;
      }

      for(size_t col_idx : {0, 1} ) {

        std::string k = metadata->column_name(col_idx);

        sframe res = fill_linear_model_sframe_from_eigen_data(
            metadata,
            col_idx,

            index_sizes[col_idx],

            include_w_term,
            index_offsets[col_idx],
            "linear_terms",
            w,

            include_V_term,
            index_offsets[col_idx],
            "factors",
            V);

        std::shared_ptr<unity_sframe> lt_sf(new unity_sframe);
        lt_sf->construct_from_sframe(res);

        ret[k] = to_variant(lt_sf);
      }
    }

    ////////////////////////////////////////////////////////////////////////////////

    // Now, do the same thing for the remaining side columns, but
    // include them as one sframe with all the "indices" given as
    // strings.
    {
      std::vector<sframe> additional_columns;
      bool include_V_term = true;
      bool include_w_term = enable_linear_features;

      switch(factor_mode) {
        case model_factor_mode::factorization_machine:
          include_V_term = true;
          break;
        case model_factor_mode::pure_linear_model:
        case model_factor_mode::matrix_factorization:
          include_V_term = false;
          break;
      }

      for(size_t col_idx = 2; col_idx < metadata->num_columns(); ++col_idx) {

        std::string k = metadata->column_name(col_idx);

        sframe res = fill_linear_model_sframe_from_eigen_data(
            metadata,
            col_idx,

            index_sizes[col_idx],

            include_w_term,
            index_offsets[col_idx],
            "linear_terms",
            w,

            include_V_term,
            index_offsets[col_idx],
            "factors",
            V);

        // Change the column type of one of them
        {

          std::shared_ptr<sarray<flexible_type> > new_x(new sarray<flexible_type>);

          new_x->open_for_write();
          new_x->set_type(flex_type_enum::STRING);
          auto it_out = new_x->get_output_iterator(0);

          std::shared_ptr<sarray<flexible_type> > old_x = res.select_column(k);
          auto reader = old_x->get_reader();
          size_t num_segments = old_x->num_segments();

          for(size_t sidx = 0; sidx < num_segments; ++sidx) {
            auto src_it     = reader->begin(sidx);
            auto src_it_end = reader->end(sidx);

            for(; src_it != src_it_end; ++src_it, ++it_out)
              *it_out = flex_string(*src_it);
          }

          new_x->close();

          res = res.remove_column(res.column_index(k));
          res = res.add_column(new_x, "index");

          std::shared_ptr<sarray<flexible_type> > name_column(
              new sarray<flexible_type>(
                  flexible_type(k), res.num_rows()));

          res = res.add_column(name_column, "feature");
        }

        additional_columns.push_back(std::move(res));
      }

      ////////////////////////////////////////////////////////////////////////////////
      // Now normalize these things

      if(!additional_columns.empty()) {
        sframe all_res = additional_columns[0];

        for(size_t i = 1; i < additional_columns.size(); ++i)
          all_res = all_res.append(additional_columns[i]);

        std::shared_ptr<unity_sframe> lt_sf(new unity_sframe);

        std::vector<std::string> names = {"feature", "index"};

        if(include_w_term)
          names.push_back("linear_terms");

        if(include_V_term)
          names.push_back("factors");

        lt_sf->construct_from_sframe(all_res.select_columns(names));

        ret["side_data"] = to_variant(lt_sf);
      }
    }

    return ret;
  }


  /** Scores all the items in scores, updating the score.  Used by the
   *  recommender system.
   */
  void score_all_items(
      std::vector<std::pair<size_t, double> >& scores,
      const std::vector<v2::ml_data_entry>& query_row,
      size_t top_k,
      const std::shared_ptr<v2::ml_data_side_features>& known_side_features) const {

    DASSERT_GE(query_row.size(), 2);
    DASSERT_EQ(query_row[USER_COLUMN_INDEX].column_index, USER_COLUMN_INDEX);
    DASSERT_EQ(query_row[ITEM_COLUMN_INDEX].column_index, ITEM_COLUMN_INDEX);

    bool has_side_features = (known_side_features != nullptr);
    bool has_additional_columns = has_side_features || (query_row.size() > 2);

    size_t user = query_row[USER_COLUMN_INDEX].index;

    // Direct it to the appropriate function
    if(factor_mode == model_factor_mode::matrix_factorization
       && !has_additional_columns
       && user < index_sizes[USER_COLUMN_INDEX]) {

      _score_all_items_simple_mf(scores, user, top_k);

    } else {
      std::vector<v2::ml_data_entry> x = query_row;

      if(has_side_features) {
        _score_all_items_general_purpose<true>(scores, std::move(x), top_k, known_side_features);
      } else {
        _score_all_items_general_purpose<false>(scores, std::move(x), top_k, nullptr);
      }
    }
  }

  // A cache of the vector values to avoid memory reallocations.
  mutable std::vector<vector_type> recommend_cache;

  /** Scoring things when it's the simple matrix factorization case.
   *  Here, we use a matrix vector product for speed.
   *
   */
  void _score_all_items_simple_mf(
      std::vector<std::pair<size_t, double> >& scores,
      size_t user,
      size_t top_k) const GL_HOT {


    size_t items_offset = index_offsets[ITEM_COLUMN_INDEX];
    size_t num_items    = index_sizes[ITEM_COLUMN_INDEX];

    size_t thread_idx = thread::thread_id();

    vector_type& cached_user_item_product = recommend_cache[thread_idx];

    cached_user_item_product.noalias() =
        V.middleRows(items_offset, num_items) * V.row(user).transpose()
        + w.segment(items_offset, num_items);

    size_t user_global_index = index_offsets[USER_COLUMN_INDEX] + user;

    DASSERT_LT(user_global_index, w.size());

    double adjustment = (w0 + w[user_global_index]);

    // The general purpose one.
    for(size_t i = 0; i < scores.size(); ++i) {

      size_t item = scores[i].first;

      // Add in any side information concerning the item to the
      // observation vector.

      double raw_score = (item < index_sizes[ITEM_COLUMN_INDEX]
                          ? cached_user_item_product[item]
                          : 0);

      scores[i].second = adjustment + raw_score;
    }

    auto p_less_than = [](const std::pair<size_t, double>& p1,
                          const std::pair<size_t, double>& p2) {
      return p1.second < p2.second;
    };

    // Now, pull off the top k elements for these.
    extract_and_sort_top_k(scores, top_k, p_less_than);

    // Now, if the loss function translates the raw scores, we need to work with that.
    if(loss_model->prediction_is_translated()) {
      for(size_t i = 0; i < scores.size(); ++i) {
        scores[i].second = loss_model->translate_fx_to_prediction(scores[i].second);
      }
    }
  }

  /**  Run the recommendations when the routine uses something more
   *   than just the straight matrix factorization.  In this case, we
   *   call the calculate_fx function to get the scores, then get the
   *   top_k out, then translate just those to the correct scores.
   */
  template <bool has_side_features>
      GL_HOT_FLATTEN
      void _score_all_items_general_purpose(
          std::vector<std::pair<size_t, double> >& scores,
          std::vector<v2::ml_data_entry>&& x,
          size_t top_k,
          const std::shared_ptr<v2::ml_data_side_features>& known_side_features) const {

    size_t thread_idx = thread::thread_id();

    // Remember the size of this vector for the rest of the rounds;
    // resizing x to this size erases any of the item data present.

    const size_t x_base_size = x.size();

    // The general purpose one.
    for(size_t i = 0; i < scores.size(); ++i) {

      size_t item = scores[i].first;

      if(has_side_features)
        x.resize(x_base_size);

      x[ITEM_COLUMN_INDEX].index = item;

      // Add in any side information concerning the item to the
      // observation vector.

      if(has_side_features)
        known_side_features->add_partial_side_features_to_row(x, ITEM_COLUMN_INDEX, item);

      // Get the raw score.
      double raw_score = calculate_fx(thread_idx, x);

      // Possibly add it to the score.
      scores[i] = {item, raw_score};
    }

    // Now, pull off the top k elements for these.
    auto p_less_than_2 = [](const std::pair<size_t, double>& p1,
                          const std::pair<size_t, double>& p2) {
      return p1.second < p2.second;
    };

    // Now, pull off the top k elements for these.
    extract_and_sort_top_k(scores, top_k, p_less_than_2);

    // Now, if the loss function translates the raw scores, we need to work with that.
    if(loss_model->prediction_is_translated()) {
      for(size_t i = 0; i < scores.size(); ++i) {
        scores[i].second = loss_model->translate_fx_to_prediction(scores[i].second);
      }
    }
  }


  mutable mutex factor_norm_lock;
  mutable bool factor_norms_computed = false;
  mutable vector_type factor_norms;

  /** Computes the cosine similarity between a particular factor
   * within a column and all the other factors within that column.
   */
  void calculate_intracolumn_similarity( vector_type& dest, size_t column_index, size_t ref_index) const {

    dest.resize(index_sizes[column_index]);

    if(V.rows() == 0) {
      dest.setZero();
      return;
    }

    // Get the factor norms if we need them.
    if(!factor_norms_computed) {
      std::lock_guard<mutex> lg(factor_norm_lock);

      if(!factor_norms_computed) {
        factor_norms.noalias() = V.rowwise().norm();
        factor_norms_computed = true;
      }
    }

    size_t start_idx = index_offsets[column_index];
    size_t block_size = index_sizes[column_index];

    dest.noalias() = V.middleRows(start_idx, block_size) * V.row(start_idx + ref_index).transpose();
    dest.array() /= (V.row(start_idx + ref_index).norm() * factor_norms.segment(start_idx, block_size).array());
  }

};

}}

#endif
