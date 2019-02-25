
#ifndef CONFIG_IFACE_H
#define CONFIG_IFACE_H

#ifdef _WIN32
__declspec(dllexport)
#endif
  int configUnusedFunc()
{
  int unused;
  return 0;
}

#endif
