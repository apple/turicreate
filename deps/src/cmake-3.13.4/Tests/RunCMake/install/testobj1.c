#ifdef _WIN32
__declspec(dllimport)
#endif
  int obj1(void);

int main(void)
{
  return obj1();
}
