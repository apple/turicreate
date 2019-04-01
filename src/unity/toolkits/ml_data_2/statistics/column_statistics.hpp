/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ML2_COLUMN_STATISTICS_H_
#define TURI_ML2_COLUMN_STATISTICS_H_

#include <flexible_type/flexible_type.hpp>
#include <logger/assertions.hpp>
#include <serialization/serialization_includes.hpp>
#include <unity/toolkits/ml_data_2/ml_data_column_modes.hpp>
#include <unity/lib/variant.hpp>

namespace turi { namespace v2 { namespace ml_data_internal {

/** Uses the factory model for saving and loading.
 */
class column_statistics {

public:

  column_statistics() = default;

  virtual ~column_statistics() = default;

  /**
   * Equality testing in subclass -- slow!  Use for
   * debugging/testing.  Upcast this to superclass to do full testing.
   */
  virtual bool is_equal(const column_statistics* other) const = 0;

  /**
   * Equality testing -- slow!  Use for debugging/testing
   */
  bool operator==(const column_statistics& other) const;

  /**
   * Inequality testing -- slow!  Use for debugging/testing
   */
  bool operator!=(const column_statistics& other) const;

  ////////////////////////////////////////////////////////////
  // Functions to access the statistics

  /** Returns the number of seen by the methods collecting the
   *  statistics.
   */
  virtual size_t num_observations() const { return size_t(-1); }

  /* The count; index here is the index obtained by one of the
   * map_value_to_index functions previously.
   */
  virtual size_t count(size_t index) const { return size_t(-1); }

  /* The mean; index here is the index obtained by one of the
   * map_value_to_index functions previously.
   */
  virtual double mean(size_t index) const { return NAN; }

  /* The variance; index here is the index obtained by one of the
   * map_value_to_index functions previously.
   */
  virtual double stdev(size_t index) const { return NAN; }

  /* The variance; index here is the index obtained by one of the
   * map_value_to_index functions previously.
   */
  virtual size_t n_positive(size_t index) const { return size_t(-1); }


  ////////////////////////////////////////////////////////////
  // Routines for updating the statistics.  This is done online, while
  // new categories are being added, etc., so we have to be

  /// Initialize the statistics -- counting, mean, and stdev
  virtual void initialize() = 0;

  /// Update categorical statistics for a batch of categorical indices.
  virtual void update_categorical_statistics(
      size_t thread_idx, const std::vector<size_t>& cat_index_vect) = 0;

  /// Update categorical statistics for a batch of real values.
  virtual void update_numeric_statistics(
      size_t thread_idx, const std::vector<double>& value_vect) = 0;

  /// Update statistics after observing a dictionary.
  virtual void update_dict_statistics(
      size_t thread_idx, const std::vector<std::pair<size_t, double> >& dict) = 0;

  /** Perform final computations on the different statistics.  Called
   *  after all the data is filled.
   */
  virtual void finalize() = 0;

  ////////////////////////////////////////////////////////////////////////////////
  // Methods for creation and serialization

    /** Returns the current version used for the serialization.
   */
  virtual size_t get_version() const = 0;

  /** Serialize the object (save).
   */
  virtual void save_impl(turi::oarchive& oarc) const = 0;

  /** Load the object.
   */
  virtual void load_version(turi::iarchive& iarc, size_t version) = 0;

  /**  The factory method for loading and instantiating the proper class
   */
  static std::shared_ptr<column_statistics> factory_create(
      const std::map<std::string, variant_type>& creation_options);

  const std::map<std::string, variant_type>& get_serialization_parameters() const {
    return creation_options;
  }

  /** One way to set the statistics.  Used by the serialization converters.
   */
  virtual void set_data(const std::map<std::string, variant_type>& params) {} 

  /** Create a copy with the index cleared.
   */
  virtual std::shared_ptr<column_statistics> create_cleared_copy() const = 0; 
   
 private:

  /** A snapshot of the options needed for creating the class.
   */
  std::map<std::string, variant_type> creation_options;

 protected:

  // Store the basic column data.  This allows us to do error checking
  // and error reporting intelligently.

  std::string column_name;
  ml_column_mode mode;
  flex_type_enum original_column_type;
  std::map<std::string, flexible_type> options;
};

}}}

////////////////////////////////////////////////////////////////////////////////
// Implement serialization for vector<std::shared_ptr<column_statistics>
// > and std::shared_ptr<column_statistics>

BEGIN_OUT_OF_PLACE_SAVE(arc, std::shared_ptr<v2::ml_data_internal::column_statistics>, m) {
  if(m == nullptr) {
    arc << false;
  } else {
    arc << true;

    // Save the version number
    size_t version = m->get_version();
    arc << version;

    // Save the model parameters as a map
    std::map<std::string, variant_type> serialization_parameters =
        m->get_serialization_parameters();

    // Save the version along with the creation options.
    serialization_parameters["version"] = to_variant(m->get_version());

    variant_deep_save(serialization_parameters, arc);

    m->save_impl(arc);
  }

} END_OUT_OF_PLACE_SAVE()


BEGIN_OUT_OF_PLACE_LOAD(arc, std::shared_ptr<v2::ml_data_internal::column_statistics>, m) {
  bool is_not_nullptr;
  arc >> is_not_nullptr;
  if(is_not_nullptr) {

    size_t version;
    arc >> version;

    std::map<std::string, variant_type> creation_options;
    variant_deep_load(creation_options, arc);

    m = v2::ml_data_internal::column_statistics::factory_create(creation_options);

    m->load_version(arc, version);

  } else {
    m = std::shared_ptr<v2::ml_data_internal::column_statistics>(nullptr);
  }
} END_OUT_OF_PLACE_LOAD()


#endif
