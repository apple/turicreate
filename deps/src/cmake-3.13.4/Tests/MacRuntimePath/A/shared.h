
#ifndef shared_h
#define shared_h

#ifdef WIN32
#  ifdef shared_EXPORTS
#    define SHARED_EXPORT __declspec(dllexport)
#  else
#    define SHARED_EXPORT __declspec(dllimport)
#  endif
#else
#  define SHARED_EXPORT
#endif

void SHARED_EXPORT shared();

#endif
