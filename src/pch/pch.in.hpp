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

// 2. Boost
#include <boost/align.hpp>
#include <boost/aligned_storage.hpp>
#include <boost/any.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/assign.hpp>
#include <boost/atomic.hpp>
#include <boost/beast.hpp>
#include <boost/bimap.hpp>
#include <boost/bind.hpp>
#include <boost/blank.hpp>
#include <boost/blank_fwd.hpp>
#include <boost/call_traits.hpp>
#include <boost/callable_traits.hpp>
#include <boost/cast.hpp>
#include <boost/cerrno.hpp>
#include <boost/checked_delete.hpp>
#include <boost/chrono.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/circular_buffer_fwd.hpp>
#include <boost/compressed_pair.hpp>
#include <boost/compute.hpp>
#include <boost/concept_archetype.hpp>
#include <boost/concept_check.hpp>
#include <boost/config.hpp>
#include <boost/contract.hpp>
#include <boost/contract_macro.hpp>
#include <boost/convert.hpp>
#include <boost/crc.hpp>
#include <boost/cregex.hpp>
#include <boost/cstdfloat.hpp>
#include <boost/cstdint.hpp>
#include <boost/cstdlib.hpp>
#include <boost/current_function.hpp>
#include <boost/cxx11_char_types.hpp>
#include <boost/date_time.hpp>
#include <boost/dll.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/dynamic_bitset_fwd.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/flyweight.hpp>
#include <boost/foreach.hpp>
#include <boost/foreach_fwd.hpp>
#include <boost/format.hpp>
#include <boost/function.hpp>
#include <boost/function_equal.hpp>
#include <boost/function_output_iterator.hpp>
#include <boost/functional.hpp>
#include <boost/generator_iterator.hpp>
#include <boost/geometry.hpp>
#include <boost/get_pointer.hpp>
#include <boost/gil.hpp>
#include <boost/hof.hpp>
#include <boost/implicit_cast.hpp>
#include <boost/indirect_reference.hpp>
#include <boost/integer.hpp>
#include <boost/integer_fwd.hpp>
#include <boost/integer_traits.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/io_fwd.hpp>
#include <boost/is_placeholder.hpp>
#include <boost/iterator.hpp>
#include <boost/iterator_adaptors.hpp>
#include <boost/last_value.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/limits.hpp>
#include <boost/local_function.hpp>
#include <boost/locale.hpp>
#include <boost/make_default.hpp>
#include <boost/make_shared.hpp>
#include <boost/make_unique.hpp>
#include <boost/math_fwd.hpp>
#include <boost/mem_fn.hpp>
#include <boost/memory_order.hpp>
#include <boost/metaparse.hpp>
#include <boost/mp11.hpp>
#include <boost/multi_array.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index_container_fwd.hpp>
#include <boost/next_prior.hpp>
#include <boost/non_type.hpp>
#include <boost/noncopyable.hpp>
#include <boost/nondet_random.hpp>
#include <boost/none.hpp>
#include <boost/none_t.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <boost/parameter.hpp>
#include <boost/phoenix.hpp>
#include <boost/pointee.hpp>
#include <boost/pointer_cast.hpp>
#include <boost/pointer_to_other.hpp>
#include <boost/polymorphic_cast.hpp>
#include <boost/polymorphic_pointer_cast.hpp>
#include <boost/preprocessor.hpp>
#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <boost/progress.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/random.hpp>
#include <boost/range.hpp>
#include <boost/ratio.hpp>
#include <boost/rational.hpp>
#include <boost/ref.hpp>
#include <boost/regex_fwd.hpp>
#include <boost/scope_exit.hpp>
#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/shared_container_iterator.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/static_assert.hpp>
#include <boost/swap.hpp>
#include <boost/thread.hpp>
#include <boost/throw_exception.hpp>
#include <boost/timer.hpp>
#include <boost/token_functions.hpp>
#include <boost/token_iterator.hpp>
#include <boost/tokenizer.hpp>
#include <boost/type.hpp>
#include <boost/type_index.hpp>
#include <boost/type_traits.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/utility.hpp>
#include <boost/variant.hpp>
#include <boost/version.hpp>
#include <boost/visit_each.hpp>
#include <boost/weak_ptr.hpp>
