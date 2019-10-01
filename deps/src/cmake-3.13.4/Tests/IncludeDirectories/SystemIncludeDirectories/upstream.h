
#ifndef UPSTREAM_H
#define UPSTREAM_H

#include "systemlib.h"

#ifdef _WIN32
__declspec(dllexport)
#endif
  int upstream();

#endif
