// clang-format off

#include "ImportLibNative.h"
#include "ImportLibMixedNative.h"

#include <iostream>

int main()
{
  std::cout << "ReferenceImportNative" << std::endl;
  ImportLibNative::Message();
  ImportLibMixedNative::Message();
};
