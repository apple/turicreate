#ifndef pcShared_h
#define pcShared_h

#ifdef _WIN32
#  ifdef pcShared_EXPORTS
#    define PC_EXPORT __declspec(dllexport)
#  else
#    define PC_EXPORT __declspec(dllimport)
#  endif
#else
#  define PC_EXPORT
#endif

PC_EXPORT const char* pcShared(void);

#endif
