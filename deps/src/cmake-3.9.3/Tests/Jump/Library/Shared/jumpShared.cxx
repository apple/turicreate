#ifdef _WIN32
#define JUMP_EXPORT __declspec(dllexport)
#else
#define JUMP_EXPORT
#endif

JUMP_EXPORT int jumpShared()
{
  return 0;
}
