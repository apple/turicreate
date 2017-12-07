/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <sketches/hyperloglog.hpp>
#include <unity/lib/gl_sarray.hpp>

namespace turi {
namespace visualization {

class hyperloglog {
  private:
    gl_sarray m_source;
    size_t m_currentIdx = 0;
    double m_estimate;
    sketches::hyperloglog m_hll;

  public:
    hyperloglog();
    void init(const gl_sarray& source);
    bool eof() const;
    double error_bound();
    double get();
}; // hyperloglog

}}
