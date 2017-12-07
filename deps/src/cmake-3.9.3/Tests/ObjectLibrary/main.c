#if defined(_WIN32) && defined(SHARED_C)
#define IMPORT_C __declspec(dllimport)
#else
#define IMPORT_C
#endif
extern IMPORT_C int b1(void);
extern IMPORT_C int b2(void);
extern IMPORT_C int c(void);
int main(void)
{
  return 0 + c() + b1() + b2();
}
