/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/data/flexible_type/flexible_type_spirit_parser.hpp>
using namespace turi;
using namespace boost::spirit;

int main(int argc, char** argv) {
  std::string s;
  std::string delimiter = ",";
  if (argc == 2) delimiter = argv[1];
  flexible_type_parser parser(delimiter);
    std::string line;
    while(std::getline(std::cin, line)) {
      if (!s.empty()) s += "\n";
      s += line;
    }
    const char* c = s.c_str();
    std::pair<flexible_type, bool> ret = parser.general_flexible_type_parse(&c, s.length());
    if (ret.second) {
      std::cout << flex_type_enum_to_name(ret.first.get_type()) << ":";
      std::cout << ret.first << "\n";
      std::cout << "Remainder: " << c << "\n";
    } else {
      std::cout << "Failed Parse\n";
    }


}

