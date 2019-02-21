#ifdef _WIN32
__declspec(dllimport)
#endif
  extern void bar();

int main()
{
  bar();
  return 0;
}
