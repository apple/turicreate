#if defined(_WIN32) || defined(WIN32) /* Win32 version */
#ifdef CMakeTestCLibraryShared_EXPORTS
#define CMakeTest_EXPORT __declspec(dllexport)
#else
#define CMakeTest_EXPORT __declspec(dllimport)
#endif
#else
/* unix needs nothing */
#define CMakeTest_EXPORT
#endif

CMakeTest_EXPORT int CsharedFunction();
