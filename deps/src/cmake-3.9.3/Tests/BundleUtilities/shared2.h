
#ifndef shared2_h
#define shared2_h

#ifdef WIN32
#ifdef shared2_EXPORTS
#define SHARED2_EXPORT __declspec(dllexport)
#else
#define SHARED2_EXPORT __declspec(dllimport)
#endif
#else
#define SHARED2_EXPORT
#endif

void SHARED2_EXPORT shared2();

#endif
