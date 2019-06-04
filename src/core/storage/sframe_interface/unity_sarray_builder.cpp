/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sframe_interface/unity_sarray_builder.hpp>
#include <core/storage/sframe_interface/unity_sarray.hpp>

namespace turi {

void unity_sarray_builder::init(size_t num_segments, size_t history_size, flex_type_enum dtype) {
  if(m_inited)
    log_and_throw("This sarray_builder has already been initialized!");

  m_sarray = std::make_shared<sarray<flexible_type>>();
  m_sarray->open_for_write(num_segments);
  m_out_iters.resize(num_segments);
  m_history.resize(num_segments);
  for(size_t i = 0; i < num_segments; ++i) {
    m_out_iters[i] = m_sarray->get_output_iterator(i);
    m_history[i] = std::make_shared<boost::circular_buffer<flexible_type>>(history_size);
  }
  m_given_dtype = dtype;
  if(dtype == flex_type_enum::UNDEFINED)
      log_and_throw("Must specify a valid type.");
  m_sarray->set_type(m_given_dtype);

  m_inited = true;
}

void unity_sarray_builder::append(const flexible_type &val, size_t segment) {
  if(!m_inited)
    log_and_throw("Must call 'init' first!");

  if(m_closed)
    log_and_throw("Cannot append values when closed.");

  if(segment >= m_out_iters.size()) {
    log_and_throw("Invalid segment number!");
  }

  m_history[segment]->push_back(val);

  auto in_type = val.get_type();
  if(in_type != flex_type_enum::UNDEFINED && in_type != m_given_dtype) {
    log_and_throw(std::string("Append failed: ") +
        flex_type_enum_to_name(in_type) + std::string(" type is "
          "incompatible with type of future SArray."));
  }

  *(m_out_iters[segment]) = val;
}

void unity_sarray_builder::append_multiple(const std::vector<flexible_type> &vals, size_t segment) {
  for(const auto &i : vals) {
    this->append(i, segment);
  }
}

flex_type_enum unity_sarray_builder::get_type() {
  return m_given_dtype;
}

std::vector<flexible_type> unity_sarray_builder::read_history(size_t num_elems, size_t segment) {
  if(!m_inited)
    log_and_throw("Must call 'init' first!");

  if(m_closed)
    log_and_throw("History is invalid when closed.");

  if(segment >= m_history.size())
    log_and_throw("Invalid segment.");

  auto history = m_history[segment];

  if(num_elems > history->size())
    num_elems = history->size();
  if(num_elems == size_t(-1))
    num_elems = history->size();

  std::vector<flexible_type> ret_vec(num_elems);

  if(num_elems == 0)
    return ret_vec;

  std::copy_n(history->rbegin(), num_elems, ret_vec.rbegin());

  return ret_vec;
}

std::shared_ptr<unity_sarray_base> unity_sarray_builder::close() {
  if(!m_inited)
    log_and_throw("Must call 'init' first!");

  if(m_closed)
    log_and_throw("Already closed.");

  m_sarray->close();
  m_closed = true;
  auto ret = std::make_shared<unity_sarray>();
  ret->construct_from_sarray(m_sarray);
  return ret;
}

} // namespace turi
