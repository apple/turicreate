// clang-format off

using namespace System;

#using <ImportLibMixed.dll>
#using <ImportLibPure.dll>
#using <ImportLibSafe.dll>
#using <ImportLibCSharp.dll>

#include "ImportLibNative.h"

int main()
{
  Console::WriteLine("ReferenceImportMixed");
  ImportLibNative::Message();
  ImportLibMixed::Message();
  ImportLibPure::Message();
  ImportLibSafe::Message();
  ImportLibCSharp::Message();
};
