/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmAlgorithms_h
#define cmAlgorithms_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cm_kwiml.h"
#include <algorithm>
#include <functional>
#include <iterator>
#include <sstream>
#include <string.h>
#include <string>
#include <utility>
#include <vector>

inline bool cmHasLiteralPrefixImpl(const std::string& str1, const char* str2,
                                   size_t N)
{
  return strncmp(str1.c_str(), str2, N) == 0;
}

inline bool cmHasLiteralPrefixImpl(const char* str1, const char* str2,
                                   size_t N)
{
  return strncmp(str1, str2, N) == 0;
}

inline bool cmHasLiteralSuffixImpl(const std::string& str1, const char* str2,
                                   size_t N)
{
  size_t len = str1.size();
  return len >= N && strcmp(str1.c_str() + len - N, str2) == 0;
}

inline bool cmHasLiteralSuffixImpl(const char* str1, const char* str2,
                                   size_t N)
{
  size_t len = strlen(str1);
  return len >= N && strcmp(str1 + len - N, str2) == 0;
}

template <typename T, size_t N>
const T* cmArrayBegin(const T (&a)[N])
{
  return a;
}
template <typename T, size_t N>
const T* cmArrayEnd(const T (&a)[N])
{
  return a + N;
}
template <typename T, size_t N>
size_t cmArraySize(const T (&)[N])
{
  return N;
}

template <typename T, size_t N>
bool cmHasLiteralPrefix(const T& str1, const char (&str2)[N])
{
  return cmHasLiteralPrefixImpl(str1, str2, N - 1);
}

template <typename T, size_t N>
bool cmHasLiteralSuffix(const T& str1, const char (&str2)[N])
{
  return cmHasLiteralSuffixImpl(str1, str2, N - 1);
}

struct cmStrCmp
{
  cmStrCmp(const char* test)
    : m_test(test)
  {
  }
  cmStrCmp(const std::string& test)
    : m_test(test)
  {
  }

  bool operator()(const std::string& input) const { return m_test == input; }

  bool operator()(const char* input) const
  {
    return strcmp(input, m_test.c_str()) == 0;
  }

private:
  const std::string m_test;
};

template <typename FwdIt>
FwdIt cmRotate(FwdIt first, FwdIt middle, FwdIt last)
{
  const typename std::iterator_traits<FwdIt>::difference_type dist =
    std::distance(middle, last);
  std::rotate(first, middle, last);
  std::advance(first, dist);
  return first;
}

template <typename Container, typename Predicate>
void cmEraseIf(Container& cont, Predicate pred)
{
  cont.erase(std::remove_if(cont.begin(), cont.end(), pred), cont.end());
}

namespace ContainerAlgorithms {

template <typename T>
struct cmIsPair
{
  enum
  {
    value = false
  };
};

template <typename K, typename V>
struct cmIsPair<std::pair<K, V> >
{
  enum
  {
    value = true
  };
};

template <typename Range,
          bool valueTypeIsPair = cmIsPair<typename Range::value_type>::value>
struct DefaultDeleter
{
  void operator()(typename Range::value_type value) const { delete value; }
};

template <typename Range>
struct DefaultDeleter<Range, /* valueTypeIsPair = */ true>
{
  void operator()(typename Range::value_type value) const
  {
    delete value.second;
  }
};

template <typename FwdIt>
FwdIt RemoveN(FwdIt i1, FwdIt i2, size_t n)
{
  FwdIt m = i1;
  std::advance(m, n);
  return cmRotate(i1, m, i2);
}

template <typename Range>
struct BinarySearcher
{
  typedef typename Range::value_type argument_type;
  BinarySearcher(Range const& r)
    : m_range(r)
  {
  }

  bool operator()(argument_type const& item) const
  {
    return std::binary_search(m_range.begin(), m_range.end(), item);
  }

private:
  Range const& m_range;
};
}

template <typename const_iterator_>
struct cmRange
{
  typedef const_iterator_ const_iterator;
  typedef typename std::iterator_traits<const_iterator>::value_type value_type;
  typedef typename std::iterator_traits<const_iterator>::difference_type
    difference_type;
  cmRange(const_iterator begin_, const_iterator end_)
    : Begin(begin_)
    , End(end_)
  {
  }
  const_iterator begin() const { return Begin; }
  const_iterator end() const { return End; }
  bool empty() const { return std::distance(Begin, End) == 0; }
  difference_type size() const { return std::distance(Begin, End); }
  cmRange& advance(KWIML_INT_intptr_t amount)
  {
    std::advance(Begin, amount);
    return *this;
  }

  cmRange& retreat(KWIML_INT_intptr_t amount)
  {
    std::advance(End, -amount);
    return *this;
  }

private:
  const_iterator Begin;
  const_iterator End;
};

typedef cmRange<std::vector<std::string>::const_iterator> cmStringRange;

class cmListFileBacktrace;
typedef cmRange<std::vector<cmListFileBacktrace>::const_iterator>
  cmBacktraceRange;

template <typename Iter1, typename Iter2>
cmRange<Iter1> cmMakeRange(Iter1 begin, Iter2 end)
{
  return cmRange<Iter1>(begin, end);
}

template <typename Range>
cmRange<typename Range::const_iterator> cmMakeRange(Range const& range)
{
  return cmRange<typename Range::const_iterator>(range.begin(), range.end());
}

template <typename Range>
void cmDeleteAll(Range const& r)
{
  std::for_each(r.begin(), r.end(),
                ContainerAlgorithms::DefaultDeleter<Range>());
}

template <typename Range>
std::string cmJoin(Range const& r, const char* delimiter)
{
  if (r.empty()) {
    return std::string();
  }
  std::ostringstream os;
  typedef typename Range::value_type ValueType;
  typedef typename Range::const_iterator InputIt;
  const InputIt first = r.begin();
  InputIt last = r.end();
  --last;
  std::copy(first, last, std::ostream_iterator<ValueType>(os, delimiter));

  os << *last;

  return os.str();
}

template <typename Range>
std::string cmJoin(Range const& r, std::string const& delimiter)
{
  return cmJoin(r, delimiter.c_str());
}

template <typename Range>
typename Range::const_iterator cmRemoveN(Range& r, size_t n)
{
  return ContainerAlgorithms::RemoveN(r.begin(), r.end(), n);
}

template <typename Range, typename InputRange>
typename Range::const_iterator cmRemoveIndices(Range& r, InputRange const& rem)
{
  typename InputRange::const_iterator remIt = rem.begin();
  typename InputRange::const_iterator remEnd = rem.end();
  const typename Range::iterator rangeEnd = r.end();
  if (remIt == remEnd) {
    return rangeEnd;
  }

  typename Range::iterator writer = r.begin();
  std::advance(writer, *remIt);
  typename Range::iterator pivot = writer;
  typename InputRange::value_type prevRem = *remIt;
  ++remIt;
  size_t count = 1;
  for (; writer != rangeEnd && remIt != remEnd; ++count, ++remIt) {
    std::advance(pivot, *remIt - prevRem);
    prevRem = *remIt;
    writer = ContainerAlgorithms::RemoveN(writer, pivot, count);
  }
  return ContainerAlgorithms::RemoveN(writer, rangeEnd, count);
}

template <typename Range, typename MatchRange>
typename Range::const_iterator cmRemoveMatching(Range& r, MatchRange const& m)
{
  return std::remove_if(r.begin(), r.end(),
                        ContainerAlgorithms::BinarySearcher<MatchRange>(m));
}

namespace ContainerAlgorithms {

template <typename Range, typename T = typename Range::value_type>
struct RemoveDuplicatesAPI
{
  typedef typename Range::const_iterator const_iterator;
  typedef typename Range::const_iterator value_type;

  static bool lessThan(value_type a, value_type b) { return *a < *b; }
  static value_type uniqueValue(const_iterator a) { return a; }
  template <typename It>
  static bool valueCompare(It it, const_iterator it2)
  {
    return **it != *it2;
  }
};

template <typename Range, typename T>
struct RemoveDuplicatesAPI<Range, T*>
{
  typedef typename Range::const_iterator const_iterator;
  typedef T* value_type;

  static bool lessThan(value_type a, value_type b) { return a < b; }
  static value_type uniqueValue(const_iterator a) { return *a; }
  template <typename It>
  static bool valueCompare(It it, const_iterator it2)
  {
    return *it != *it2;
  }
};
}

template <typename Range>
typename Range::const_iterator cmRemoveDuplicates(Range& r)
{
  typedef typename ContainerAlgorithms::RemoveDuplicatesAPI<Range> API;
  typedef typename API::value_type T;
  std::vector<T> unique;
  unique.reserve(r.size());
  std::vector<size_t> indices;
  size_t count = 0;
  const typename Range::const_iterator end = r.end();
  for (typename Range::const_iterator it = r.begin(); it != end;
       ++it, ++count) {
    const typename std::vector<T>::iterator low = std::lower_bound(
      unique.begin(), unique.end(), API::uniqueValue(it), API::lessThan);
    if (low == unique.end() || API::valueCompare(low, it)) {
      unique.insert(low, API::uniqueValue(it));
    } else {
      indices.push_back(count);
    }
  }
  if (indices.empty()) {
    return end;
  }
  return cmRemoveIndices(r, indices);
}

template <typename Range>
std::string cmWrap(std::string const& prefix, Range const& r,
                   std::string const& suffix, std::string const& sep)
{
  if (r.empty()) {
    return std::string();
  }
  return prefix + cmJoin(r, suffix + sep + prefix) + suffix;
}

template <typename Range>
std::string cmWrap(char prefix, Range const& r, char suffix,
                   std::string const& sep)
{
  return cmWrap(std::string(1, prefix), r, std::string(1, suffix), sep);
}

template <typename Range, typename T>
typename Range::const_iterator cmFindNot(Range const& r, T const& t)
{
  return std::find_if(r.begin(), r.end(),
                      std::bind1st(std::not_equal_to<T>(), t));
}

template <typename Range>
cmRange<typename Range::const_reverse_iterator> cmReverseRange(
  Range const& range)
{
  return cmRange<typename Range::const_reverse_iterator>(range.rbegin(),
                                                         range.rend());
}

template <class Iter>
std::reverse_iterator<Iter> cmMakeReverseIterator(Iter it)
{
  return std::reverse_iterator<Iter>(it);
}

inline bool cmHasSuffix(const std::string& str, const std::string& suffix)
{
  if (str.size() < suffix.size()) {
    return false;
  }
  return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline void cmStripSuffixIfExists(std::string& str, const std::string& suffix)
{
  if (cmHasSuffix(str, suffix)) {
    str.resize(str.size() - suffix.size());
  }
}

#endif
