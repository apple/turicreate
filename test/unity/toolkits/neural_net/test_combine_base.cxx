/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_combine

#include <ml/neural_net/combine_mock.hpp>

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>

namespace turi {
namespace neural_net {
namespace {

BOOST_AUTO_TEST_CASE(test_demand) {
  // Test Demand::Unlimited()
  Demand unlimited = Demand::Unlimited();
  TS_ASSERT(unlimited.IsUnlimited());
  TS_ASSERT(!unlimited.IsNone());

  // Test Demand::None()
  Demand none = Demand::None();
  TS_ASSERT(!none.IsUnlimited());
  TS_ASSERT(none.IsNone());
  TS_ASSERT_EQUALS(none.max(), 0);

  // Test Demand::Demand(int) with zero max
  Demand zero = Demand(0);
  TS_ASSERT(!zero.IsUnlimited());
  TS_ASSERT(zero.IsNone());
  TS_ASSERT_EQUALS(zero.max(), 0);

  // Test Demand::Demand(int) with positive max
  Demand seven = Demand(7);
  TS_ASSERT(!seven.IsUnlimited());
  TS_ASSERT(!seven.IsNone());
  TS_ASSERT_EQUALS(seven.max(), 7);

  // Test Demand::Add(...) with unlimited and unlimited
  Demand unlimited_plus_unlimited = Demand::Unlimited().Add(unlimited);
  TS_ASSERT(unlimited_plus_unlimited.IsUnlimited());
  TS_ASSERT(!unlimited_plus_unlimited.IsNone());
  TS_ASSERT_LESS_THAN(unlimited_plus_unlimited.max(), 0);

  // Test Demand::Add(...) with unlimited and limited
  Demand unlimited_plus_seven = Demand::Unlimited().Add(seven);
  TS_ASSERT(unlimited_plus_seven.IsUnlimited());
  TS_ASSERT(!unlimited_plus_seven.IsNone());
  TS_ASSERT_LESS_THAN(unlimited_plus_unlimited.max(), 0);

  // Test Demand::Add(...) with limited and unlimited
  Demand seven_plus_unlimited = Demand(7).Add(unlimited);
  TS_ASSERT(seven_plus_unlimited.IsUnlimited());
  TS_ASSERT(!seven_plus_unlimited.IsNone());
  TS_ASSERT_LESS_THAN(unlimited_plus_unlimited.max(), 0);

  // Test Demand::Add(...) with limited and limited
  Demand seven_plus_seven = Demand(7).Add(seven);
  TS_ASSERT(!seven_plus_seven.IsUnlimited());
  TS_ASSERT(!seven_plus_seven.IsNone());
  TS_ASSERT_EQUALS(seven_plus_seven.max(), 14);

  // Test Demand::Decrement() with unlimited
  Demand unlimited_minus_one = Demand::Unlimited().Decrement();
  TS_ASSERT(unlimited_minus_one.IsUnlimited());
  TS_ASSERT(!unlimited_minus_one.IsNone());
  TS_ASSERT_LESS_THAN(unlimited_plus_unlimited.max(), 0);

  // Test Demand::Decrement() with none
  Demand none_minus_one = Demand::None().Decrement();
  TS_ASSERT(!none_minus_one.IsUnlimited());
  TS_ASSERT(none_minus_one.IsNone());
  TS_ASSERT_EQUALS(none_minus_one.max(), 0);

  // Test Demand::Decrement() with positive
  Demand seven_minus_one = Demand(7).Decrement();
  TS_ASSERT(!seven_minus_one.IsUnlimited());
  TS_ASSERT(!seven_minus_one.IsNone());
  TS_ASSERT_EQUALS(seven_minus_one.max(), 6);
}

BOOST_AUTO_TEST_CASE(test_completion) {
  // Test Completion::Finished()
  Completion completion = Completion::Finished();
  TS_ASSERT(completion.IsFinished());
  TS_ASSERT(completion.failure() == nullptr);

  // Test Completion::Failure(...) returns !IsFinished()...
  const std::string kExceptionMessage = "Test exception";
  try {
    throw std::runtime_error(kExceptionMessage);
  } catch (...) {
    completion = Completion::Failure(std::current_exception());
  }
  TS_ASSERT(!completion.IsFinished());

  // ... and preserves the exception passed on construction
  try {
    std::rethrow_exception(completion.failure());
  } catch (const std::runtime_error& e) {
    TS_ASSERT_EQUALS(e.what(), kExceptionMessage);
  }
}

}  // namespace
}  // namespace neural_net
}  // namespace turi
