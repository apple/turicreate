/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_ALL_OPERATORS_H_
#define TURI_SFRAME_QUERY_ALL_OPERATORS_H_

#include <sframe_query_engine/operators/append.hpp>
#include <sframe_query_engine/operators/binary_transform.hpp>
#include <sframe_query_engine/operators/constant.hpp>
#include <sframe_query_engine/operators/logical_filter.hpp>
#include <sframe_query_engine/operators/project.hpp>
#include <sframe_query_engine/operators/range.hpp>
#include <sframe_query_engine/operators/sarray_source.hpp>
#include <sframe_query_engine/operators/sframe_source.hpp>
#include <sframe_query_engine/operators/transform.hpp>
#include <sframe_query_engine/operators/generalized_transform.hpp>
#include <sframe_query_engine/operators/union.hpp>
#include <sframe_query_engine/operators/generalized_union_project.hpp>
#include <sframe_query_engine/operators/reduce.hpp>
#ifdef TC_HAS_PYTHON
#include <sframe_query_engine/operators/lambda_transform.hpp>
#endif
#include <sframe_query_engine/operators/optonly_identity_operator.hpp>
#include <sframe_query_engine/operators/ternary_operator.hpp>


#endif /* TURI_SFRAME_QUERY_ALL_OPERATORS_H_ */
