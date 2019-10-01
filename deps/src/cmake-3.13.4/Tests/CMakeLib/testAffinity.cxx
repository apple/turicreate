/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmAffinity.h"

#include <cstddef>
#include <iostream>
#include <set>

int main()
{
  std::set<size_t> cpus = cmAffinity::GetProcessorsAvailable();
  if (!cpus.empty()) {
    std::cout << "CPU affinity mask count is '" << cpus.size() << "'.\n";
  } else {
    std::cout << "CPU affinity not supported on this platform.\n";
  }
  return 0;
}
