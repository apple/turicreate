/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "io_buffer.hpp"

using namespace turi::visualization;

std::string io_buffer::read() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_queue.empty()) {
    return "";
  }
  auto ret = m_queue.front();
  m_queue.pop();
  return ret;
}

void io_buffer::write(const std::string& str) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_queue.push(str);
}

size_t io_buffer::size() const {
  return m_queue.size();
}
