/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#pragma once
#include "cmConfigure.h" // IWYU pragma: keep

#include <cstddef>
#include <set>

namespace cmAffinity {

std::set<size_t> GetProcessorsAvailable();
}
