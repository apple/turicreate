
#ifndef SYSTEMLIB_H
#define SYSTEMLIB_H

#if defined(_WIN32) || defined(__CYGWIN__)
#  define systemlib_EXPORT __declspec(dllexport)
#else
#  define systemlib_EXPORT
#endif

struct systemlib_EXPORT SystemStruct
{
  SystemStruct();

  void someMethod()
  {
    int unused;
    // unused warning not issued when this header is used as a system header.
  }
};

#endif
