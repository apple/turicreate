#ifdef _WIN32
#  define JUMP_IMPORT __declspec(dllimport)
#else
#  define JUMP_IMPORT extern
#endif

extern int jumpStatic();
JUMP_IMPORT int jumpShared();
int main()
{
  return jumpShared() && jumpShared();
}
