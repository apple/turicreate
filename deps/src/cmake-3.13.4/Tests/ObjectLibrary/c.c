#if defined(_WIN32) && defined(Cshared_EXPORTS)
#  define EXPORT_C __declspec(dllexport)
#else
#  define EXPORT_C
#endif

extern int a1(void);
extern int a2(void);
extern int b1(void);
extern int b2(void);
EXPORT_C int c(void)
{
  return 0 + a1() + a2() + b1() + b2();
}
