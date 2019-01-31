/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#include <boost/algorithm/string.hpp>
#include <fault/query_object_server_manager.hpp>

int main(int argc, char** argv) {
  if (argc < 5) {
    std::cout << "Usage: " << argv[0] << " [zkhosts] [prefix] [object name list / file]\n"
              << "             [server program] [replicacount = 2] [object capacity = 32]\n"
              << "             [initial max masters = inf] \n";
    std::cout << "zkhosts is a comma seperated list of zookeeper servers\n";
    std::cout << "Object name list can be a comma separated list of names, or a filename\n";
    std::cout << "initial max masters is the maximum number of masters to create at the start\n";
    std::cout << "replicacount, object capacity, and max masters are optional\n";
    return 0;
  }


  // parse zkhosts
  std::vector<std::string> zkhosts;
  boost::split(zkhosts, argv[1],
               boost::is_any_of(", "), boost::token_compress_on);

  // parse prefix
  std::string prefix = argv[2];

  // parse object space
  std::vector<std::string> masterspace;
  std::ifstream fin(argv[3]);
  if (fin.good()) {
    std::cout << "Interpreting " << argv[3] << " as a file\n";
    while (!fin.eof()) {
      std::string s;
      fin >> s;
      boost::trim(s);
      if (!s.empty()) masterspace.push_back(s);
    }
  } else {
    std::cout << "Interpreting " << argv[3] << " as a comma seperated list\n";
    boost::split(masterspace, argv[3],
                 boost::is_any_of(", "), boost::token_compress_on);
  }

  // parse program
  std::string program = argv[4];

  size_t replicacount = 2;
  size_t objectcap = 32;
  size_t max_masters = (size_t)(-1);

  if (argc >= 6) replicacount = atoi(argv[5]);
  if (argc >= 7) objectcap = atoi(argv[6]);
  if (argc >= 8) max_masters = atoi(argv[7]);

  libfault::query_object_server_manager manager(program, replicacount, objectcap);
  manager.register_zookeeper(zkhosts, prefix);
  manager.set_all_object_keys(masterspace);
  std::cout << "\n\n\n";
  manager.start(max_masters);
  while(1) {
    std::cout << "l: list objects\n";
    std::cout << "s [object]: stop managing object\n";
    std::cout << "q: quit\n";
    char command;
    std::cin >> command;
    if (command == 'q') break;
    else if (command == 'l'){
      manager.print_all_object_names();
    } else if(command == 's') {
      std::string objectname;
      std::cin >> objectname;

      manager.stop_managing_object(objectname);
      std::cout << "\n";
    }
  }
  manager.stop();
}
