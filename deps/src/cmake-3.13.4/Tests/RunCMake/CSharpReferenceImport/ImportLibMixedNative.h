#pragma once

#ifdef ImportLibMixed_EXPORTS
#  define mixedAPI __declspec(dllexport)
#else
#  define mixedAPI __declspec(dllimport)
#endif

class mixedAPI ImportLibMixedNative
{
public:
  static void Message();
};
