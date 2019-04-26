
struct
#ifdef _WIN32
  __declspec(dllexport)
#endif
    OnlyPlainLib1
{
  OnlyPlainLib1();

  int GetResult();

private:
  int result;
};
