/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

// Pre-compiled header (source)
// Used as input to the compiler to generate a pre-compiled header.

// Includes:
// 1. C++ Standard Library

// 1a. Utilities Library
#include <cstdlib>
#include <csignal>
#include <csetjmp>
#include <cstdarg>
#include <typeinfo>
#include <typeindex>
#include <type_traits>
#include <bitset>
#include <functional>
#include <utility>
#include <ctime>
#include <chrono>
#include <cstddef>
#include <initializer_list>
#include <tuple>
// 1a. Utilities Library: Dynamic memory management
#include <new>
#include <memory>
#include <scoped_allocator>
// 1a. Utilities Library: Numeric limits
#include <climits>
#include <cfloat>
#include <cstdint>
#include <cinttypes>
#include <limits>
// 1a. Utilities Library: Error handling
#include <exception>
#include <stdexcept>
#include <cassert>
#include <system_error>
#include <cerrno>

// 1b. Strings library
#include <cctype>
#include <cwctype>
#include <cstring>
#include <cwchar>
#include <string>

// 1c. Containers library
#include <array>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>

// 1d. Iterators library
#include <iterator>

// 1f. Algorithms library
#include <algorithm>

// 1g. Numerics library
#include <cmath>
#include <complex>
#include <valarray>
#include <random>
#include <numeric>
#include <ratio>
#include <cfenv>

// 1h. Input/output library
#include <iosfwd>
#include <ios>
#include <istream>
#include <ostream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <streambuf>
#include <cstdio>

// 1i. Localization library
#include <locale>
#include <clocale>

// 1j. Regular Expressions library
#include <regex>

// 1k. Atomic Operations library
#include <atomic>

// 1l. Thread support library
#include <thread>
#include <mutex>
#include <future>
#include <condition_variable>