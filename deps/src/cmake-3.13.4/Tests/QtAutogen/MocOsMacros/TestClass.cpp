#include "TestClass.hpp"
#include <iostream>

// -- Mac
#ifndef Q_OS_MAC
void TestClass::MacNotDef()
{
  std::cout << "MacNotDef\n";
}
#else
void TestClass::MacNotDefElse()
{
  std::cout << "MacNotDefElse\n";
}
#endif

#ifdef Q_OS_MAC
void TestClass::MacDef()
{
  std::cout << "MacDef\n";
}
#else
void TestClass::MacDefElse()
{
  std::cout << "MacDefElse\n";
}
#endif

// -- Unix
#ifndef Q_OS_UNIX
void TestClass::UnixNotDef()
{
  std::cout << "UnixNotDef\n";
}
#else
void TestClass::UnixNotDefElse()
{
  std::cout << "UnixNotDefElse\n";
}
#endif

#ifdef Q_OS_UNIX
void TestClass::UnixDef()
{
  std::cout << "UnixDef\n";
}
#else
void TestClass::UnixDefElse()
{
  std::cout << "UnixDefElse\n";
}
#endif

// -- Windows
#ifndef Q_OS_WIN
void TestClass::WindowsNotDef()
{
  std::cout << "WindowsNotDef\n";
}
#else
void TestClass::WindowsNotDefElse()
{
  std::cout << "WindowsNotDefElse\n";
}
#endif

#ifdef Q_OS_WIN
void TestClass::WindowsDef()
{
  std::cout << "WindowsDef\n";
}
#else
void TestClass::WindowsDefElse()
{
  std::cout << "WindowsDefElse\n";
}
#endif
