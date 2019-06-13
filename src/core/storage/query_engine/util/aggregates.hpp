/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_ENGINE_UTIL_AGGREGATES_HPP
#define TURI_SFRAME_QUERY_ENGINE_UTIL_AGGREGATES_HPP
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
#include <core/storage/query_engine/operators/reduce.hpp>
#include <core/storage/query_engine/planning/planner.hpp>

namespace turi {

/**
 * \ingroup sframe_query_engine
 * \addtogroup Utilities Utilities
 * \{
 */
namespace query_eval {
/**
 * Implements a generic aggregator where a function is called on each add method.
 *
 * Any attempt to combine, or save, will result in a failure.
 */
template <typename T, typename AggregateFunctionType>
class generic_aggregator: public group_aggregate_value {
 public:
   generic_aggregator():value(T()) { }
   generic_aggregator(AggregateFunctionType fn, const T& t):
       fn(fn), initial_value(t), value(t) { }

   /// Returns a new empty instance of sum with the same type
   group_aggregate_value* new_instance() const {
     generic_aggregator* ret =
         new generic_aggregator(fn, initial_value);
     return ret;
   }

   /// Adds a new element to be summed
   void add_element_simple(const flexible_type& flex) {
     fn(flex, value);
   }

   /// Emits the result
   flexible_type emit() const {
     // we just emit strings
     std::stringstream strm;
     oarchive oarc(strm);
     oarc << value;
     return strm.str();
   }

   void combine(const group_aggregate_value& flex) {
     ASSERT_TRUE(false);
   }

   bool support_type(flex_type_enum) const {
     ASSERT_TRUE(false);
     return false;
   }

   /// Serializer
   void save(oarchive& oarc) const {
     ASSERT_TRUE(false);
   }

   /// Deserializer
   void load(iarchive& iarc) {
     ASSERT_TRUE(false);
   }

   std::string name() const {
     ASSERT_TRUE(false);
     return "";
   }
 private:
   AggregateFunctionType fn;
   T initial_value;
   T value;
};

/**
 * Performs a reduction on input in parallel, this function decides the
 * degree of parallelism, usually depend on number of CPUs.
 *
 * \param input The iterator supports parallel batch reading of items
 * \param reduce_fn The reduction function to use. This must be of the form
 *                 bool f(const array_value_type&, reduction_type&).
 * \param init The initial value to use in the reduction
 *
 * \tparam ResultType The type of the intermediate result. Must be serializable
 * \tparam ReduceFunctionType The result type of each reduction.
 * \tparam AggregateFunctionType The type of the reduce function
 *
 *
 */
template <typename ResultType,
         typename ReduceFunctionType,
          typename AggregateFunctionType>
ResultType reduce(
  std::shared_ptr<planner_node> input,
  ReduceFunctionType reduce_fn,
  AggregateFunctionType aggregate_fn,
  ResultType init = ResultType()) {

  generic_aggregator<ResultType, ReduceFunctionType> agg(reduce_fn, init);
  auto output = op_reduce::make_planner_node(input, agg, flex_type_enum::STRING);
  sframe sf = planner().materialize(output);
  auto sfreader = sf.get_reader(1);
  auto iter = sfreader->begin(0);
  ResultType result = init;
  ResultType curval;
  while(iter != sfreader->end(0)) {
    // data is serialized in an archive:w
    std::string st = (*iter)[0];
    iarchive iarc(st.c_str(), st.length());
    iarc >> curval;
    aggregate_fn(curval, result);
    ++iter;
  }
  return result;
}


/// \}

} // namespace query_eval
} // namespace turi
#endif
