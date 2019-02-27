#include "TestClass.hpp"
#include <QtGlobal>

int main()
{
  TestClass a;
#ifdef Q_OS_MAC
  a.MacNotDefElse();
  a.MacDef();
#else
  a.MacNotDef();
  a.MacDefElse();
#endif

#ifdef Q_OS_UNIX
  a.UnixNotDefElse();
  a.UnixDef();
#else
  a.UnixNotDef();
  a.UnixDefElse();
#endif

#ifdef Q_OS_WIN
  a.WindowsNotDefElse();
  a.WindowsDef();
#else
  a.WindowsNotDef();
  a.WindowsDefElse();
#endif

  return 0;
}
