
#ifndef SYSTEMLIB_H
#define SYSTEMLIB_H

#ifdef _WIN32
__declspec(dllexport)
#endif
  int systemlib();

#ifdef _WIN32
__declspec(dllexport)
#endif
  int unusedFunc()
{
  int unused;
  return systemlib();
}

#endif
