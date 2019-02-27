// clang-format off

using namespace System;

#using <ImportLibMixed.dll>
#using <ImportLibPure.dll>
#using <ImportLibSafe.dll>
#using <ImportLibCSharp.dll>

int main()
{
  Console::WriteLine("ReferenceImportPure");
  ImportLibMixed::Message();
  ImportLibPure::Message();
  ImportLibSafe::Message();
  ImportLibCSharp::Message();
};
