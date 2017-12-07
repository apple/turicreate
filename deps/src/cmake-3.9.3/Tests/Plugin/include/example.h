#ifndef example_h
#define example_h

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(example_exe_EXPORTS)
#define EXAMPLE_EXPORT __declspec(dllexport)
#else
#define EXAMPLE_EXPORT __declspec(dllimport)
#endif
#else
#define EXAMPLE_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

EXAMPLE_EXPORT int example_exe_function(void);

#ifdef __cplusplus
}
#endif

#endif
