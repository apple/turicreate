#pragma once

#ifdef ImportLibNative_EXPORTS
#  define nativeAPI __declspec(dllexport)
#else
#  define nativeAPI __declspec(dllimport)
#endif

class nativeAPI ImportLibNative
{
public:
  static void Message();
};
