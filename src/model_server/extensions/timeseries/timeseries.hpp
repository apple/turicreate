/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TIMESERIES_H
#define TIMESERIES_H

#include <string>
#include <vector>
#include <deque>
#include <iostream>
#include <cassert>
#include <math.h>
#include <core/logging/logger.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_data/group_aggregate_value.hpp>
#include <model_server/lib/toolkit_class_macros.hpp>
#include <core/data/sframe/gl_sarray.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <model_server/lib/extensions/model_base.hpp>
#include <model_server/extensions/timeseries/grouped_timeseries.hpp>
#include <model_server/extensions/timeseries/interpolate_value.hpp>


namespace turi {
namespace timeseries {


EXPORT gl_sarray date_range(const flexible_type &start_time,
                            const flexible_type &end_time,
                            const flexible_type & period);

typedef std::shared_ptr<interpolator_value> interpolator_type;
class grouped_timeseries;

/***
 * gl_timeseries is the fundamental data-structure to hold multi-variate
 * timeseries data.  It is backed by a single gl_sframe and some meta-data.
 *
 ***/
class EXPORT gl_timeseries : public model_base {
  public:
      virtual ~gl_timeseries();

  protected:
      static constexpr size_t TIMESERIES_VERSION = 0;
      gl_sframe m_sframe;                 // The backend gl_sframe
      bool m_initialized = false;

      void _check_if_initialized() const {
        if(!m_initialized)
         throw std::string("Timeseries is not initialized.");
      }

  public:
      std::vector<std::string> m_value_col_names;
      std::string m_index_col_name;

      gl_sframe get_sframe() const {
        return m_sframe;
      }
      void set_sframe(gl_sframe sf) {
        m_sframe = sf;
      }

      std::string get_index_col_name() const {
        return m_index_col_name;
      }

      flex_type_enum get_index_col_type() const {
        return m_sframe[m_index_col_name].dtype();
      }

      void set_index_col_name(std::string index_col) {
        m_index_col_name = index_col;
      }

      std::vector<std::string> get_value_col_names() const {
        return m_value_col_names;
      }
      void set_value_col_names(std::vector<std::string> val_names) {
        m_value_col_names = val_names;
      }
      /**
       * Get a version for the object.
       **/
      size_t get_version() const {
        return TIMESERIES_VERSION;
      }

      /**
       * Serializes gl_timeseries object. Must save the object to the file
       * format version matching that of get_version()
       **/
      void save_impl(oarchive& oarc) const;

      /**
       * Loads a gl_timeseries object previously saved at a particular version
       * number. Should raise an exception on failure.
       **/
      void load_version(iarchive& iarc, size_t version);

      void init(const gl_sframe & _input_sf,const std::string & _name, bool
          is_sorted=false,std::vector<int64_t>  ranges={-1,-1});

      /**
       * resample operator does down/up sampling.
       *
       * \param[in] period      : Period to resample to (in seconds).
       * \param[in] operators   : Operators for aggregation.
       * \param[in] fill_method : Interpolation scheme.
       * \param[in] label       : The timestamp recorded in the output
       *                          TimeSeries to determine which	654 end point
       *                          (left or right) to use to denote the time
       *                          slice.
       * \param[in] closed      : Determines which side of the interval in the
       *                          time slice is closed. Must be ['left' or
       *                          'right']
       **/
      gl_timeseries resample(
          const flex_float& period,
          const std::map<std::string,
                             aggregate::groupby_descriptor_type>& operators,
          interpolator_type interpolation_fn =
                        get_builtin_interpolator("__builtin__none__"),
          const std::string& label = "left",
          const std::string& closed = "right") const;

      /**
       * Python wrapper for resampling. This function is the one exposed via the
       * SDK to python.
       *
       * \note The reason for this is because the aggregate, and interpolation types
       *       are not objects that are registered via Python.
       *
       * \note Since the SDK does not support more than 6 arguments, the
       *       downsample_params, and upsample_params have been combined.
       *
       *
       *       downsample_params : (ds_columns, ds_output_columns, ds_ops)
       *       upsample_params   : (up_op)
       */
      gl_timeseries resample_wrapper(
              double period,
              const flex_list& downsample_params,
              const flex_list& upsample_params,
              const std::string& label,
              const std::string& close) const;

      /**
       * shift the index column of the gl_timeseries by number of seconds.
       **/
      gl_timeseries tshift(const flex_float & delta);

      /**
       * Shift the non-index columns in the TimeSeries object by specified
       * number of steps.
       * The rows at the boundary with no values anymore are replaced by None
       * values.
       **/
      gl_timeseries shift(const int64_t & steps);

      gl_timeseries slice(const flexible_type &start_time,
                          const flexible_type &end_time,
                          const std::string &closed) const;


      /**
       * join this TimeSeries with the 'other_ts' TimeSeries on their index
       * column.
       *
       * other_ts: the other TimeSeries object.
       * how: how to join two TimeSeries. Accepted methods are 'inner','outer',
       *        and 'left'
       * index_column_name: the new name for the index column of the output
       *                    TimeSeries.
       **/
      gl_timeseries index_join(const gl_timeseries& other_ts, const
          std::string& how, const std::string& index_column_name);

      /**
       * union this TimeSeries with the 'other_ts' TimeSeries.
       **/
      gl_timeseries ts_union(const gl_timeseries& other_ts);

      gl_grouped_timeseries group(std::vector<std::string> key_columns);
      void add_column(const gl_sarray& data, const std::string& name="");
      void remove_column(const std::string& name);

      BEGIN_CLASS_MEMBER_REGISTRATION("_Timeseries")
      REGISTER_CLASS_MEMBER_FUNCTION(gl_timeseries::tshift, "delta")
      REGISTER_CLASS_MEMBER_FUNCTION(gl_timeseries::shift, "steps")
      REGISTER_CLASS_MEMBER_FUNCTION(gl_timeseries::slice, "start_time",
          "end_time", "closed")
      REGISTER_CLASS_MEMBER_FUNCTION(gl_timeseries::init, "_input_sf",
          "_name", "is_sorted", "ranges")
      REGISTER_CLASS_MEMBER_FUNCTION(gl_timeseries::index_join, "other_ts",
          "how", "index_column_name")
      REGISTER_CLASS_MEMBER_FUNCTION(gl_timeseries::group, "key_columns")
      REGISTER_CLASS_MEMBER_FUNCTION(gl_timeseries::add_column, "data","name")
      REGISTER_CLASS_MEMBER_FUNCTION(gl_timeseries::remove_column, "name")
      REGISTER_CLASS_MEMBER_FUNCTION(gl_timeseries::ts_union, "other_ts")
      REGISTER_CLASS_MEMBER_FUNCTION(gl_timeseries::resample_wrapper, "period",
          "downsample_params", "upsample_params", "left", "close")

      REGISTER_GETTER("sframe", gl_timeseries::get_sframe)
      REGISTER_SETTER("sframe", gl_timeseries::set_sframe)
      REGISTER_GETTER("value_col_names", gl_timeseries::get_value_col_names)
      REGISTER_SETTER("value_col_names", gl_timeseries::set_value_col_names)
      REGISTER_GETTER("index_col_name", gl_timeseries::get_index_col_name)
      REGISTER_SETTER("index_col_name", gl_timeseries::set_index_col_name)

      END_CLASS_MEMBER_REGISTRATION
  };


} // timeseries
} // turicreate

#endif
