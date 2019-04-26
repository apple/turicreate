#ifdef _WIN32
#  ifdef testcxx2_EXPORTS
#    define CM_TEST_LIB_EXPORT __declspec(dllexport)
#  else
#    define CM_TEST_LIB_EXPORT __declspec(dllimport)
#  endif
#else
#  define CM_TEST_LIB_EXPORT
#endif

class CM_TEST_LIB_EXPORT LibCxx2Class
{
public:
  static float Method();
};
