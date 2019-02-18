
#ifndef framework_h
#define framework_h

#ifdef WIN32
#  ifdef framework_EXPORTS
#    define FRAMEWORK_EXPORT __declspec(dllexport)
#  else
#    define FRAMEWORK_EXPORT __declspec(dllimport)
#  endif
#else
#  define FRAMEWORK_EXPORT
#endif

void FRAMEWORK_EXPORT framework();

#endif
