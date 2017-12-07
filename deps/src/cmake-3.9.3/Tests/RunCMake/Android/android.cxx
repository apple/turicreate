#include "android.h"

#ifndef STL_NONE
#include <cmath>
#include <cstdio>
#ifndef STL_SYSTEM
#include <exception>
#include <typeinfo>
#ifndef STL_STLPORT
#include <cxxabi.h>
#endif
#ifndef STL_GABI
#include <iostream>
#include <string>
#endif
#endif
#endif

int main()
{
#if !defined(STL_NONE)
  // Require -lm implied by linking as C++.
  std::printf("%p\n", static_cast<double (*)(double)>(&std::sin));
#endif
#if defined(STL_NONE)
  return 0;
#elif defined(STL_SYSTEM)
  return 0;
#else
  try {
    delete (new int);
  } catch (std::exception const& e) {
#if defined(STL_GABI)
    e.what();
    typeid(e).name();
#else
    std::cerr << e.what() << std::endl;
    std::cerr << typeid(e).name() << std::endl;
#endif
  }
#if defined(STL_GABI)
  return 0;
#else
  std::string s;
  return static_cast<int>(s.size());
#endif
#endif
}
