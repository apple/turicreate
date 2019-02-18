#ifdef _WIN32
#  ifdef bar_EXPORTS
#    define BAR_EXPORT __declspec(dllexport)
#  else
#    define BAR_EXPORT __declspec(dllimport)
#  endif
#else
#  define BAR_EXPORT
#endif
