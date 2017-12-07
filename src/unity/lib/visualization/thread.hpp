/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef __TC_VISUALIZATION_THREAD
#define __TC_VISUALIZATION_THREAD

#include <functional>

namespace turi {
  namespace visualization {
    void run_thread(std::function<void()> func);
  }
} // ::turi::visualization

#endif // __TC_VISUALIZATION_THREAD
