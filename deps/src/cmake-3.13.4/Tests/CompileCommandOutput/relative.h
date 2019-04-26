#if defined(_WIN32)
#  ifdef test2_EXPORTS
#    define TEST2_EXPORT __declspec(dllexport)
#  else
#    define TEST2_EXPORT __declspec(dllimport)
#  endif
#else
#  define TEST2_EXPORT
#endif

TEST2_EXPORT void relative();
