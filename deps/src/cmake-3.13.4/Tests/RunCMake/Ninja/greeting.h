#if defined(_WIN32) && !defined(GREETING_STATIC)
__declspec(dllimport)
#endif
  void greeting(void);
