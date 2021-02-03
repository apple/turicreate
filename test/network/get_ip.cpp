/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <iostream>
#include <core/system/platform/network/net_util.hpp>

using namespace turi;

int main(int argc, char **argv) {
  std::cerr << get_local_ip_as_str(true);
}
