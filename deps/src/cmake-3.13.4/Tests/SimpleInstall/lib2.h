#ifdef _WIN32
#ifdef test2_EXPORTS
#define CM_TEST_LIB_EXPORT __declspec(dllexport)
#else
#define CM_TEST_LIB_EXPORT __declspec(dllimport)
#endif
#else
#define CM_TEST_LIB_EXPORT
#endif

CM_TEST_LIB_EXPORT float Lib2Func();
