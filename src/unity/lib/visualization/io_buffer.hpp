/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef __TC_VISUALIZATION_IO_BUFFER
#define __TC_VISUALIZATION_IO_BUFFER

#include <mutex>
#include <string>
#include <queue>

namespace turi {
  namespace visualization {
    class io_buffer {
      private:
        std::mutex m_mutex;
        std::queue<std::string> m_queue;

      public:
        std::string read();
        void write(const std::string&);
        size_t size() const;
    };

  }
}

#endif // __TC_VISUALIZATION_IO_BUFFER
