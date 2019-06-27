/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TRY_FINALLY_H_
#define TURI_TRY_FINALLY_H_

#include <vector>
#include <functional>

namespace turi {

/**
 * \ingroup util
 * Class to use the garuanteed destructor call of a scoped variable
 *   to simulate a try...finally block.  This is the standard C++ way
 *   of doing that.
 *
 *   Use:
 *
 *   When you want to ensure something is executed, even when
 *   exceptions are present, create a scoped_finally structure and add
 *   the appropriate cleanup functions to it.  When this structure
 *   goes out of scope, either due to an exception or the code leaving
 *   that scope, these functions are executed.
 *
 *   This is useful for cleaning up locks, etc.
 */
class scoped_finally {
 public:

  scoped_finally()
  {}

  scoped_finally(std::vector<std::function<void()> > _f_v)
      : f_v(_f_v)
  {}

  scoped_finally(std::function<void()> _f)
      : f_v{_f}
  {}

  void add(std::function<void()> _f) {
    f_v.push_back(_f);
  }

  void execute_and_clear() {
    for(size_t i = f_v.size(); (i--) != 0;) {
      f_v[i]();
    }
    f_v.clear();
  }

  ~scoped_finally() {
    execute_and_clear();
  }

 private:
  std::vector<std::function<void()> > f_v;
};

}







#endif /* _TRY_FINALLY_H_ */
