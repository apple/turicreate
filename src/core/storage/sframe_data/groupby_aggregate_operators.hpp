/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_GROUPBY_AGGREGATE_OPERATORS_HPP
#define TURI_SFRAME_GROUPBY_AGGREGATE_OPERATORS_HPP
#include <core/storage/sframe_data/group_aggregate_value.hpp>
#include <ml/sketches/streaming_quantile_sketch.hpp>
namespace turi {


/**
 * \ingroup sframe_physical
 * \addtogroup groupby_aggregate Groupby Aggregation
 * \{
 */

/**
 * Groupby Aggregation Operators
 */
namespace groupby_operators {
/**
 * Implements a vector sum aggregator
 */
class vector_sum : public group_aggregate_value {
 public:
  /// Returns a new empty instance of sum with the same type
  group_aggregate_value* new_instance() const {
    vector_sum* ret = new vector_sum;
    ret->set_input_type(value.get_type());
    return ret;
  }

  /// Adds a new element to be summed
  void add_element_simple(const flexible_type& flex) {
    // add everything except undefined
    if (!failure && flex.get_type() != flex_type_enum::UNDEFINED) {
      DASSERT_EQ((int)flex.get_type(), (int)value.get_type());
      // If this is first element being added, just copy since intialized vector
      // is size 0 and you cannot add vectors of different size.
      if (!init){
        value = flex;
        init = true;
      } else{
          if (flex.size() != value.size()){
            failure = true;
          } else {
            value += flex;
          }
      }
    }
  }

  /// combines two partial sums
  void combine(const group_aggregate_value& other) {
    const vector_sum& other_casted = dynamic_cast<const vector_sum&>(other);
    // If you add vectors of different lengths, make result be undefined.
    if (!other_casted.failure && !failure){
      if (!init) {
        // I am not initialized.
        (*this) = other_casted;
      } else if (other_casted.init) {
        if (value.size() != other_casted.value.size()) {
          failure = true;
        } else {
        value += other_casted.value;
        }
      }
    }
  }


  /// Emits the sum result
  flexible_type emit() const {
    if (failure) {
      return flexible_type(flex_type_enum::UNDEFINED);
    }
    return value;
  }

  /// The types supported by the sum
  bool support_type(flex_type_enum type) const {
    return type == flex_type_enum::VECTOR || type == flex_type_enum::ND_VECTOR;
  }

  /// The input type to be summed
  flex_type_enum set_input_type(flex_type_enum type) {
    value.reset(type);
    return type;
  }

  /// Name of the class
  std::string name() const {
    return "Vector Sum";
  }

  /// Serializer
  void save(oarchive& oarc) const {
    oarc << value << init << failure;
  }

  /// Deserializer
  void load(iarchive& iarc) {
    iarc >> value >> init >>failure;
  }

 private:
  flexible_type value = flexible_type(flex_type_enum::VECTOR);
  bool failure = false;
  bool init = false;
};


/**
 * Implements a sum aggregator
 */
class sum : public group_aggregate_value {
 public:
  /// Returns a new empty instance of sum with the same type
  group_aggregate_value* new_instance() const {
    sum* ret = new sum;
    ret->set_input_type(value.get_type());
    return ret;
  }

  /// Adds a new element to be summed
  void add_element_simple(const flexible_type& flex) {
    // add everything except undefined
    if (flex.get_type() != flex_type_enum::UNDEFINED){
      DASSERT_EQ((int)flex.get_type(), (int)value.get_type());
      value += flex;
    }
  }

  /// combines two partial sums
  void combine(const group_aggregate_value& other) {
    value += dynamic_cast<const sum&>(other).value;
  }


  /// Emits the sum result
  flexible_type emit() const {
    return value;
  }

  /// The types supported by the sum
  bool support_type(flex_type_enum type) const {
    return type == flex_type_enum::INTEGER ||
           type == flex_type_enum::FLOAT;
  }

  /// The input type to be summed
  flex_type_enum set_input_type(flex_type_enum type) {
    value.reset(type);
    return type;
  }

  /// Name of the class
  std::string name() const {
    return "Sum";
  }

  /// Serializer
  void save(oarchive& oarc) const {
    oarc << value;
  }

  /// Deserializer
  void load(iarchive& iarc) {
    iarc >> value;
  }

 private:
  flexible_type value;
};


/**
 * Implements a min aggregator
 */
class min: public group_aggregate_value {
 public:
  /// Returns a new empty instance of min with the same type
  group_aggregate_value* new_instance() const {
    min* ret = new min;
    ret->set_input_type(value.get_type());
    return ret;
  }

  /// Adds a new element
  void add_element_simple(const flexible_type& flex) {
    // add everything except undefined
    if (flex.get_type() != flex_type_enum::UNDEFINED) {
      DASSERT_EQ((int)flex.get_type(), (int)value.get_type());
      if (!init) {
        init = true;
        value = flex;
      } else {
        if (value > flex)
          value = flex;
      }
    }
  }

  /// combines two partial mins
  void combine(const group_aggregate_value& other) {
    const min& _other = dynamic_cast<const min&>(other);
    if (_other.init) {
      if (!init) {
        init = true;
        value = _other.value;
      } else {
        if (value > _other.value)
          value = _other.value;
      }
    }
  }

  /// Emits the min result
  flexible_type emit() const {
    if (!init) return FLEX_UNDEFINED;
    else return value;
  }

  /// The types supported by the min
  bool support_type(flex_type_enum type) const {
    return type == flex_type_enum::INTEGER ||
           type == flex_type_enum::FLOAT ||
           type == flex_type_enum::DATETIME;
  }

  /// The input type
  flex_type_enum set_input_type(flex_type_enum type) {
    value.reset(type);
    return type;
  }

  /// Name of the class
  std::string name() const {
    return "Min";
  }

  /// Serializer
  void save(oarchive& oarc) const {
    oarc << value << init;
  }

  /// Deserializer
  void load(iarchive& iarc) {
    iarc >> value >> init;
  }

 private:
  flexible_type value;
  bool init = false;
};

/**
 *
 * Implemenets a argmin aggregator which is used with arg_func
 *
 */

class argmin: public group_aggregate_value {
 public:
  /// Returns a new empty instance of argmin with the same type
  group_aggregate_value* new_instance() const {
    argmin* ret = new argmin;
    return ret;
  }


  void add_element(const std::vector<flexible_type>& values) {
    // add everything except undefined
    DASSERT_TRUE(values.size() > 0);
    if (values[0].get_type() != flex_type_enum::UNDEFINED) {
      if (!init) {
        vec_value = values;
        init = true;
      } else {
        if (vec_value[0] > values[0])  vec_value = values;
      }
    }
  }

  /// Adds a new element
  void add_element_simple(const flexible_type& flex) {
    throw "argmin does not support add_element_simple with one value";
  }

  /// combines two partial argmins
  void combine(const group_aggregate_value& other) {
    const argmin& _other = dynamic_cast<const argmin&>(other);
    if (_other.init) {
      if (!init) {
        vec_value = _other.vec_value;
        init = true;
      } else {
        if (vec_value[0] > _other.vec_value[0]) vec_value = _other.vec_value;
      }
    }
  }

  /// Emits the argmin result
  flexible_type emit() const {
    if (vec_value.empty()) return FLEX_UNDEFINED;
    else return flexible_type(vec_value[1]);
  }

  /// The types supported by the argmin
  bool support_type(flex_type_enum type) const {
    return type == flex_type_enum::INTEGER ||
           type == flex_type_enum::FLOAT ||
           type == flex_type_enum::DATETIME;
  }

  /// The input type to be argmined
  flex_type_enum set_input_types(const std::vector<flex_type_enum>& types) {
     DASSERT_TRUE(types.size() == 2);
     // return type of the supporting column.¬
     return types[1];
  }


  flex_type_enum set_input_type(flex_type_enum type) {
    throw ("set_input_type is not supported for argmin");
  }

  /// Name of the class
  std::string name() const {
    return "argmin";
  }

  /// Serializer
  void save(oarchive& oarc) const {
    oarc << vec_value << init;
  }

  /// Deserializer
  void load(iarchive& iarc) {
    iarc >> vec_value >> init;
  }

 private:
  // The input type to be maxed.
  std::vector<flexible_type> vec_value;
  bool init = false;
};

/**
 *
 * Implemenets a argmax aggregator which is used with arg_func
 *
 */

class argmax: public group_aggregate_value {
 public:
  /// Returns a new empty instance of argmax with the same type
  group_aggregate_value* new_instance() const {
    argmax* ret = new argmax;
    return ret;
  }


  void add_element(const std::vector<flexible_type>& values) {
    // add everything except undefined
    DASSERT_TRUE(values.size() > 0);
    if (values[0].get_type() != flex_type_enum::UNDEFINED) {
      if (!init) {
        vec_value = values;
        init = true;
      } else {
        if (vec_value[0] < values[0])  vec_value = values;
      }
    }
  }

  /// Adds a new element
  void add_element_simple(const flexible_type& flex) {
    throw "argmax does not support add_element_simple with one value";
  }

  /// combines two partial argmaxes
  void combine(const group_aggregate_value& other) {
    const argmax& _other = dynamic_cast<const argmax&>(other);
    if (_other.init) {
      if (!init) {
        vec_value = _other.vec_value;
        init = true;
      } else {
        if (vec_value[0] < _other.vec_value[0]) vec_value = _other.vec_value;
      }
    }
  }

  /// Emits the argmax result
  flexible_type emit() const {
    if (vec_value.empty()) return FLEX_UNDEFINED;
    else return flexible_type(vec_value[1]);
  }

  /// The types supported by the argmax
  bool support_type(flex_type_enum type) const {
    return type == flex_type_enum::INTEGER ||
           type == flex_type_enum::FLOAT ||
           type == flex_type_enum::DATETIME;
  }

  /// The input type to be argmaxed
  flex_type_enum set_input_types(const std::vector<flex_type_enum>& types) {
    DASSERT_TRUE(types.size() == 2);
    // return type of the supporting column.
    return types[1];
  }

  flex_type_enum set_input_type(flex_type_enum type) {
    throw ("set_input_type is not supported for argmax");
  }

  /// Name of the class
  std::string name() const {
    return "argmax";
  }

  /// Serializer
  void save(oarchive& oarc) const {
    oarc << vec_value << init;
  }

  /// Deserializer
  void load(iarchive& iarc) {
    iarc >> vec_value >> init;
  }

 private:
  // The input type to be maxed.
  std::vector<flexible_type> vec_value;
  bool init = false;
};

/**
 * Implements a max aggregator
 */
class max: public group_aggregate_value {
 public:
  /// Returns a new empty instance of max with the same type
  group_aggregate_value* new_instance() const {
    max* ret = new max;
    ret->set_input_type(value.get_type());
    return ret;
  }

  /// Adds a new element
  void add_element_simple(const flexible_type& flex) {
    // add everything except undefined
    if (flex.get_type() != flex_type_enum::UNDEFINED) {
      DASSERT_EQ((int)flex.get_type(), (int)value.get_type());
      if (!init) {
        value = flex;
        init = true;
      } else {
        if  (value < flex)  value = flex;
      }
    }
  }

  /// combines two partial maxes
  void combine(const group_aggregate_value& other) {
    const max& _other = dynamic_cast<const max&>(other);
    if (_other.init) {
      if (!init) {
        value = _other.value;
        init = true;
      } else {
        if (value < _other.value) value = _other.value;
      }
    }
  }

  /// Emits the max result
  flexible_type emit() const {
    if (!init) return FLEX_UNDEFINED;
    else return value;
  }

  /// The types supported by the max
  bool support_type(flex_type_enum type) const {
    return type == flex_type_enum::INTEGER ||
           type == flex_type_enum::FLOAT ||
           type == flex_type_enum::DATETIME;
  }

  /// The input type to be maxed
  flex_type_enum set_input_type(flex_type_enum type) {
    value.reset(type);
    return type;
  }

  /// Name of the class
  std::string name() const {
    return "Max";
  }

  /// Serializer
  void save(oarchive& oarc) const {
    oarc << value << init;
  }

  /// Deserializer
  void load(iarchive& iarc) {
    iarc >> value >> init;
  }

 private:
  flexible_type value;
  bool init = false;
};

/**
 * Implements a count aggregator
 */
class count: public group_aggregate_value {
 public:
  /// Returns a new empty instance of count
  group_aggregate_value* new_instance() const {
    count* ret = new count;
    return ret;
  }

  void add_element_simple(const flexible_type& flex) {
    ++value;
  }

  /// combines two partial counts
  void combine(const group_aggregate_value& other) {
    value += dynamic_cast<const count&>(other).value;
  }

  /// Emits the count result
  flexible_type emit() const {
    return flexible_type(value);
  }

  /// The types supported by the count. (everything)
  bool support_type(flex_type_enum type) const {
    return true;
  }

  /// The input type
  flex_type_enum set_input_types(const std::vector<flex_type_enum>& types) {
    DASSERT_TRUE(types.size() == 0);
    return flex_type_enum::INTEGER;
  }

  flex_type_enum set_input_type(flex_type_enum type) {
    throw ("set_input_type is not supported for count");
  }

  /// Name of the class
  std::string name() const {
    return "Count";
  }

  /// Serializer
  void save(oarchive& oarc) const {
    oarc << value;
  }

  /// Deserializer
  void load(iarchive& iarc) {
    iarc >> value;
  }

 private:
  size_t value = 0;
};

/**
 * Implements a count non-null aggregator
 */
class non_null_count: public group_aggregate_value {
 public:
  /// Returns a new empty instance of count
  group_aggregate_value* new_instance() const {
    non_null_count* ret = new non_null_count;
    return ret;
  }

  void add_element_simple(const flexible_type& flex) {
    if(flex.get_type() != flex_type_enum::UNDEFINED)
      ++value;
  }

  /// combines two partial counts
  void combine(const group_aggregate_value& other) {
    value += dynamic_cast<const non_null_count&>(other).value;
  }

  /// Emits the count result
  flexible_type emit() const {
    return flexible_type(value);
  }

  /// The types supported by the count. (everything)
  bool support_type(flex_type_enum type) const {
    return true;
  }

  /// The input type
  flex_type_enum set_input_types(const std::vector<flex_type_enum>& types) {
    DASSERT_TRUE(types.size() == 0);
    return flex_type_enum::INTEGER;
  }

  flex_type_enum set_input_type(flex_type_enum type) {
    return flex_type_enum::INTEGER;
  }

  /// Name of the class
  std::string name() const {
    return "Count";
  }

  /// Serializer
  void save(oarchive& oarc) const {
    oarc << value;
  }

  /// Deserializer
  void load(iarchive& iarc) {
    iarc >> value;
  }

 private:
  size_t value = 0;
};

/**
 * Implements a vector average aggregator
 */
class vector_average: public group_aggregate_value {
 public:
  /// Returns a new empty instance of count
  group_aggregate_value* new_instance() const {
    vector_average* ret = new vector_average;
    ret->set_input_type(value.get_type());
    return ret;
  }

  /// Adds a new element to be counted
  void add_element_simple(const flexible_type& flex) {
   if (!failure && flex.get_type() != flex_type_enum::UNDEFINED) {
      // Copy if not initialized.
      if (!init){
        ++count;
        value = flex;
        init = true;
      } else{
          if (flex.size() != value.size()){
            failure = true;
          } else {
              ++count;
              // Use recurrence relation of mean to prevent overflow
              value += (flex - value)/double(count);
          }
      }
    }
  }

  /// combines two partial counts
  void combine(const group_aggregate_value& other) {
    const vector_average& other_casted = dynamic_cast<const vector_average&>(other);
    // Set to UNDEFINED if there are different vec sizes.
    if (!other_casted.failure && !failure){
      if (!init){
        (*this) = other_casted;
      } else if (other_casted.init){
        if (value.size() != other_casted.value.size()) {
          failure = true;
        } else {
          //weighted mean
          value = ((value * count) + (other_casted.value
                * other_casted.count)) / (count + other_casted.count);
          count += other_casted.count;
        }
      }
    }
  }

  /// Emits the count result
  flexible_type emit() const {
    if (failure) {
      return flexible_type(flex_type_enum::UNDEFINED);
    }
    return value;
  }

  /// The types supported by the count. (everything)
  bool support_type(flex_type_enum type) const {
    return type == flex_type_enum::VECTOR || type == flex_type_enum::ND_VECTOR;
  }

  /// The input type
  flex_type_enum set_input_type(flex_type_enum type) {
    value.reset(type);
    return type;

  }

  /// Name of the class
  std::string name() const {
    return "Vector Avg";
  }

  /// Serializer
  void save(oarchive& oarc) const {
    oarc << value << count << init << failure;
  }

  /// Deserializer
  void load(iarchive& iarc) {
    iarc >> value >> count >> init >> failure;
  }

 private:
  flexible_type value = flexible_type(flex_type_enum::VECTOR);
  bool init = false;
  bool failure = false;
  size_t count = 0;

};

/**
 * Implements an average aggregator
 */
class average: public group_aggregate_value {
 public:
  /// Returns a new empty instance of count
  group_aggregate_value* new_instance() const {
    average* ret = new average;
    return ret;
  }

  /// Adds a new element to be counted
  void add_element_simple(const flexible_type& flex) {
    if (flex != FLEX_UNDEFINED) {
      ++count;
      // Use recurrence relation of mean to prevent overflow
      value += ((double)flex - value)/double(count);
    }
  }

  /// combines two partial counts
  void combine(const group_aggregate_value& other) {
    const average& other_casted = dynamic_cast<const average&>(other);
        //weighted mean
    if (count + other_casted.count > 0){
      value = ((value * count) + (other_casted.value
                * other_casted.count)) / (count + other_casted.count);
      count += other_casted.count;
    }
  }

  /// Emits the count result
  flexible_type emit() const {
    if (count == 0) return FLEX_UNDEFINED;
    else return value;
  }

  /// The types supported by the count. (everything)
  bool support_type(flex_type_enum type) const {
    return (type == flex_type_enum::INTEGER ||
            type == flex_type_enum::FLOAT);
  }

  /// The input type
  flex_type_enum set_input_type(flex_type_enum type) {
    return flex_type_enum::FLOAT;
  }

  /// Name of the class
  std::string name() const {
    return "Avg";
  }

  /// Serializer
  void save(oarchive& oarc) const {
    oarc << value << count;
  }

  /// Deserializer
  void load(iarchive& iarc) {
    iarc >> value >> count;
  }

 private:
  double value = 0;
  size_t count = 0;
};


/**
 * Impelments the variance operator. Algorithm adapted from
 * http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Parallel_algorithm
 */
class variance : public group_aggregate_value {
 public:
  /// Returns a new empty instance of count
  virtual group_aggregate_value* new_instance() const {
    variance* ret = new variance;
    return ret;
  }

  /// Adds a new element to be counted
  void add_element_simple(const flexible_type& flex) {
    if (flex != FLEX_UNDEFINED) {
      ++count;
      double delta = (double)flex - mean;
      mean += delta / count;
      M2 += delta * ((double)flex - mean);
    }
  }

  /// combines two partial counts
  void combine(const group_aggregate_value& other) {
    const variance& _other = dynamic_cast<const variance&>(other);
    if (_other.count == 0) {
      return;
    } else if (count == 0) {
      mean = _other.mean;
      count = _other.count;
      M2 = _other.M2;
    } else {
      double delta = _other.mean - mean;
      mean = ((mean * count) + (_other.mean * _other.count)) / (count + _other.count);
      M2 += _other.M2 + delta * delta * _other.count * count / (count + _other.count);
      count += _other.count;
    }
  }

  /// Emits the count result
  virtual flexible_type emit() const {
    return count <= 1 ? flexible_type(0.0) : flexible_type(M2 / (count));
  }

  /// The types supported by the count.
  bool support_type(flex_type_enum type) const {
    return (type == flex_type_enum::INTEGER ||
            type == flex_type_enum::FLOAT);
  }

  /// The input type
  flex_type_enum set_input_type(flex_type_enum type) {
    return flex_type_enum::FLOAT;
  }

  /// Name of the class
  virtual std::string name() const {
    return "Var";
  }

  /// Serializer
  void save(oarchive& oarc) const {
    oarc << count << mean << M2;
  }

  /// Deserializer
  void load(iarchive& iarc) {
    iarc >> count >> mean >> M2;
  }

  virtual void print(std::ostream& os) const {
    os << this->name() << "("
       << "value = " << this->emit() << ", "
       << "count = " << this->count << ", "
       << "mean = "  << this->mean << ", "
       << "M2 = "    << this->M2
       << ")";
  }

 protected:
  size_t count = 0;
  double mean = 0;
  double M2 = 0;
};


class stdv : public variance {
 public:
  group_aggregate_value* new_instance() const override {
    variance* ret = new stdv;
    return ret;
  }

  /// Name of the class
  virtual std::string name() const override {
    return "Stdv";
  }

  /// Emits the count result
  virtual flexible_type emit() const override {
    return flexible_type(std::sqrt((double)(variance::emit())));
  }
};



/**
 * Impelments the quantile operator.
 */
class quantile : public group_aggregate_value {
 public:

  /**
   * Used to initialize the operator. To set the configuration for
   * what quantiles to query.
   */
  void init(const std::vector<double>& quantiles_to_query) {
    m_quantiles = quantiles_to_query;
  }

  /** Returns a new empty instance of quantile with the same set
   *  of quantiles to be queried
   */
  group_aggregate_value* new_instance() const {
    quantile* ret = new quantile;
    ret->m_quantiles = m_quantiles;
    return ret;
  }

  /// Adds a new element
  void add_element_simple(const flexible_type& flex) {
    if (flex != FLEX_UNDEFINED) {
      m_sketch.add((double)(flex));
    }
  }

  /// Done adding elements
  void partial_finalize() {
    m_sketch.substream_finalize();
  }

  /// combines two partial quantile sketches
  void combine(const group_aggregate_value& other) {
    const quantile& other_quantile = dynamic_cast<const quantile&>(other);
    m_sketch.combine(other_quantile.m_sketch);
  }

  /// Emits the desired quantiles
  virtual flexible_type emit() const {
    m_sketch.combine_finalize();
    flexible_type ret(flex_type_enum::VECTOR);
    for (size_t i = 0; i < m_quantiles.size(); ++i) {
      ret.push_back(m_sketch.query_quantile(m_quantiles[i]));
    }
    return ret;
  }

  /// The types supported by the quantile sketch (int, float)
  bool support_type(flex_type_enum type) const {
    return (type == flex_type_enum::INTEGER ||
            type == flex_type_enum::FLOAT);
  }

  /// The input type
  flex_type_enum set_input_type(flex_type_enum type) {
    return flex_type_enum::VECTOR;
  }

  /// Name of the class
  virtual std::string name() const {
    return "Quantiles";
  }

  /// Serializer
  void save(oarchive& oarc) const {
    oarc << m_quantiles << m_sketch;
  }

  /// Deserializer
  void load(iarchive& iarc) {
    iarc >> m_quantiles >> m_sketch;
  }

 private:
  std::vector<double> m_quantiles;
  mutable sketches::streaming_quantile_sketch<double> m_sketch;
};


/**
 * Implements an aggregator that convert two values from two column
 * into a key/value
 * value inside a dictionary
 */
class zip_dict : public group_aggregate_value {
 public:
  /// Returns a new empty instance of sum with the same type
  group_aggregate_value* new_instance() const {
    zip_dict* ret = new zip_dict;
    return ret;
  }

  void add_element(const std::vector<flexible_type>& values) {
    DASSERT_TRUE(values.size() == 2);

    if (values[0].get_type() != flex_type_enum::UNDEFINED) {
      m_value.insert(std::make_pair(values[0], values[1]));
    } else {
      m_missing_value = true;
    }
  }

  void add_element_simple(const flexible_type& flex) {
    throw "zip_dict does not support add_element_simple with one value";
  }

  /// combines two partial zip
  void combine(const group_aggregate_value& other) {
    auto v = dynamic_cast<const zip_dict&>(other);
    m_missing_value |= v.m_missing_value;

    if (!m_missing_value) {
      m_value.insert(v.m_value.begin(), v.m_value.end());
    }
  }

  /// Emits the zip result
  flexible_type emit() const {
    // emit None if all we got is missing key
    if (m_missing_value && m_value.size() == 0) {
      return flex_dict();
    } else {
      flex_dict ret;
      ret.insert(ret.end(), m_value.begin(), m_value.end());
      return ret;
    }
  }

  /// The types supported by the zip
  bool support_type(flex_type_enum type) const {
    return true;
  }

  flex_type_enum set_input_types(const std::vector<flex_type_enum>& types) {
    DASSERT_TRUE(types.size() == 2);
    return flex_type_enum::DICT;
  }

  flex_type_enum set_input_type(flex_type_enum type) {
    throw ("set_input_type is not supported for zip_dict");
  }

  /// Name of the class
  std::string name() const {
    return "Dict";
  }

  /// Serializer
  void save(oarchive& oarc) const {
    oarc << m_missing_value <<  m_value;
  }

  /// Deserializer
  void load(iarchive& iarc) {
    iarc >> m_missing_value >> m_value;
  }

 private:
  std::map<flexible_type, flexible_type> m_value;
  bool m_missing_value = false;
};


/**
 * Implements an aggregator that combine values from mutiple rows into one list value
 */
class zip_list : public group_aggregate_value {
 public:
  /// Returns a new empty instance of sum with the same type
  group_aggregate_value* new_instance() const {
    zip_list* ret = new zip_list;
    ret->m_is_float = m_is_float;
    return ret;
  }

  void add_element_simple(const flexible_type& flex) {
    if (flex == FLEX_UNDEFINED) {
      m_missing_value = true;
    } else {
      m_value.push_back(flex);
    }
  }

  /// combines two partial zip
  void combine(const group_aggregate_value& other) {
    auto v = dynamic_cast<const zip_list&>(other);
    m_missing_value |= v.m_missing_value;
    std::copy(v.m_value.begin(), v.m_value.end(), back_inserter(m_value));
  }

  /// Emits the zip result
  flexible_type emit() const {
    if (m_missing_value && m_value.size() == 0) {
      if (m_is_float) return flex_vec();
      else return flex_list();
    } else {
      if (m_is_float) {
        return flex_vec(m_value.begin(), m_value.end());
      } else {
        return flex_list(std::move(m_value));
      }
    }
  }

  /// The types supported by the zip
  bool support_type(flex_type_enum type) const {
    return true;
  }

  flex_type_enum set_input_types(const std::vector<flex_type_enum>& types) {
    if (types[0] == flex_type_enum::FLOAT) {
      m_is_float = true;
      return flex_type_enum::VECTOR;
    } else {
      m_is_float = false;
      return flex_type_enum::LIST;
    }
  }

  flex_type_enum set_input_type(flex_type_enum type) {
    throw ("set_input_type is not supported for zip_list");
  }

  /// Name of the class
  std::string name() const {
    return "List";
  }

  /// Serializer
  void save(oarchive& oarc) const {
    oarc << m_missing_value << m_is_float <<  m_value;
  }

  /// Deserializer
  void load(iarchive& iarc) {
    iarc >> m_missing_value >> m_is_float >> m_value;
  }

 private:
  std::vector<flexible_type> m_value;
  bool m_missing_value = false;
  bool m_is_float;
};

/**
 * Implements a select one aggregator.
 *
 * This will select one occurance of the given column. There is no guarantee
 * which one will be selected. It depends on how the groupby is implemented.
 * I believe it will select the 1st one though.
 */
class select_one: public group_aggregate_value {
 public:

  /// Returns a new empty instance of select_nth with the same type
  group_aggregate_value* new_instance() const {
    return new select_one;
  }

  /// Adds a new element to be summed
  void add_element_simple(const flexible_type& flex) {
    if (!m_has_value) {
      m_value = flex;
      m_has_value = true;
    }
  }

  /// combines two partial sums
  void combine(const group_aggregate_value& other) { }

  /// Emits the sum result
  flexible_type emit() const {
    if (m_has_value == false) return FLEX_UNDEFINED;
    else return m_value;
  }

  /// The types supported by the sum
  bool support_type(flex_type_enum type) const {
    return true;
  }

  /// The input type to be summed
  flex_type_enum set_input_type(flex_type_enum type) {
    return type;
  }

  /// Name of the class
  std::string name() const {
    return "Select One";
  }

  /// Serializer
  void save(oarchive& oarc) const {
    oarc << m_has_value << m_value;
  }

  /// Deserializer
  void load(iarchive& iarc) {
    iarc >> m_has_value >> m_value;
  }

 private:
  flexible_type m_value;
  bool m_has_value = false;
};


/**
 * Implements an aggregator that computes the exact number of unique elements
 */
class count_distinct: public group_aggregate_value {
 public:
  /// Returns a new empty instance of sum with the same type
  group_aggregate_value* new_instance() const {
    count_distinct* ret = new count_distinct;
    return ret;
  }

  void add_element_simple(const flexible_type& flex) {
    m_values.insert(flex);
  }

  /// combines two partial zip
  void combine(const group_aggregate_value& other) {
    auto& v = dynamic_cast<const count_distinct&>(other);
    m_values.insert(v.m_values.begin(), v.m_values.end());
  }

  /// Emits the zip result
  flexible_type emit() const {
    return m_values.size();
  }

  /// The types supported by the zip
  bool support_type(flex_type_enum type) const {
    return true;
  }

  flex_type_enum set_input_type(flex_type_enum type) {
    return flex_type_enum::INTEGER;
  }

  /// Name of the class
  std::string name() const {
    return "Count Distinct";
  }

  /// Serializer
  void save(oarchive& oarc) const {
    oarc << m_values;
  }

  /// Deserializer
  void load(iarchive& iarc) {
    iarc >> m_values;
  }

 protected:
  std::unordered_set<flexible_type> m_values;
};

/**
 * Implements an aggregator that keeps track of the unique elements
 */
class distinct: public count_distinct {
 public:

  group_aggregate_value* new_instance() const override {
    distinct* ret = new distinct;
    return ret;
  }

  /// Emits the zip result
  flexible_type emit() const override {
    flex_list ret(m_values.size());
    size_t i = 0;
    for (const auto& k: m_values) {
      ret[i] = k;
      i++;
    }
    return ret;
  }

  flex_type_enum set_input_type(flex_type_enum type) override {
    return flex_type_enum::LIST;
  }

  /// Name of the class
  std::string name() const override {
    return "Distinct";
  }

};

/**
 * Implements an aggregator that computes frequncies for each unique value.
 */
class frequency_count: public group_aggregate_value {
 public:
  /// Returns a new empty instance of sum with the same type
  group_aggregate_value* new_instance() const {
    frequency_count* ret = new frequency_count;
    return ret;
  }

  void add_element_simple(const flexible_type& flex) {
    m_values[flex] += 1;
  }

  /// combines two partial zip
  void combine(const group_aggregate_value& other) {
    auto& v = dynamic_cast<const frequency_count&>(other);
    for (const auto& kvp : v.m_values) {
      const auto& key = kvp.first;
      const auto& value = kvp.second;
      if (m_values.find(key) != m_values.end()) {
        // combine both counts
        m_values[key] += value;
      } else {
        m_values.insert(kvp);
      }
    }
  }

  /// Emits the zip result
  flexible_type emit() const {
    flex_dict ret(m_values.size());
    size_t i = 0;
    for (const auto& kvp: m_values) {
      ret[i] = {kvp.first, flex_int(kvp.second)};
      i++;
    }
    return ret;
  }

  bool support_type(flex_type_enum type) const {
    return (type == flex_type_enum::INTEGER||
            type == flex_type_enum::STRING);
  }

  flex_type_enum set_input_type(flex_type_enum type) {
    return flex_type_enum::DICT;
  }

  /// Name of the class
  std::string name() const {
    return "Frequency Count";
  }

  /// Serializer
  void save(oarchive& oarc) const {
    oarc << m_values;
  }

  /// Deserializer
  void load(iarchive& iarc) {
    iarc >> m_values;
  }

 protected:
  std::unordered_map<flexible_type, size_t> m_values;
};

} // namespace groupby_operators

/// \}
} // namespace turi
#endif //TURI_SFRAME_GROUPBY_AGGREGATE_OPERATORS_HPP
