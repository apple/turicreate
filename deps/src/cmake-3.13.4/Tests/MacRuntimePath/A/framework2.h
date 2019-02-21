
#ifndef framework2_h
#define framework2_h

#ifdef WIN32
#  ifdef framework2_EXPORTS
#    define FRAMEWORK2_EXPORT __declspec(dllexport)
#  else
#    define FRAMEWORK2_EXPORT __declspec(dllimport)
#  endif
#else
#  define FRAMEWORK2_EXPORT
#endif

void FRAMEWORK2_EXPORT framework2();

#endif
