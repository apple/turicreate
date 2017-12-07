/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#include "kwsysPrivate.h"
#include KWSYS_HEADER(hash_map.hxx)
#include KWSYS_HEADER(hash_set.hxx)

// Work-around CMake dependency scanning limitation.  This must
// duplicate the above list of headers.
#if 0
#include "hash_map.hxx.in"
#include "hash_set.hxx.in"
#endif

#include <iostream>

#if defined(_MSC_VER)
#pragma warning(disable : 4786)
#endif

#if defined(__sgi) && !defined(__GNUC__)
#pragma set woff 1468 /* inline function cannot be explicitly instantiated */
#endif

template class kwsys::hash_map<const char*, int>;
template class kwsys::hash_set<int>;

static bool test_hash_map()
{
  typedef kwsys::hash_map<const char*, int> mtype;
  mtype m;
  const char* keys[] = { "hello", "world" };
  m[keys[0]] = 1;
  m.insert(mtype::value_type(keys[1], 2));
  int sum = 0;
  for (mtype::iterator mi = m.begin(); mi != m.end(); ++mi) {
    std::cout << "Found entry [" << mi->first << "," << mi->second << "]"
              << std::endl;
    sum += mi->second;
  }
  return sum == 3;
}

static bool test_hash_set()
{
  typedef kwsys::hash_set<int> stype;
  stype s;
  s.insert(1);
  s.insert(2);
  int sum = 0;
  for (stype::iterator si = s.begin(); si != s.end(); ++si) {
    std::cout << "Found entry [" << *si << "]" << std::endl;
    sum += *si;
  }
  return sum == 3;
}

int testHashSTL(int, char* [])
{
  bool result = true;
  result = test_hash_map() && result;
  result = test_hash_set() && result;
  return result ? 0 : 1;
}
