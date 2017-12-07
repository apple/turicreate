/***********************************************************\
  Written by: Richard Bateman (taxilian)

  Based on the default np_macmain.cpp from FireBreath
  http://firebreath.googlecode.com

  This file has been stripped to prevent it from accidently
  doing anything useful.
\***********************************************************/

#include <stdio.h>

typedef void (*NPP_ShutdownProcPtr)(void);
typedef short NPError;

#pragma GCC visibility push(default)

struct NPNetscapeFuncs;
struct NPPluginFuncs;

extern "C" {
NPError NP_Initialize(NPNetscapeFuncs* browserFuncs);
NPError NP_GetEntryPoints(NPPluginFuncs* pluginFuncs);
NPError NP_Shutdown(void);
}

#pragma GCC visibility pop

void initPluginModule()
{
}

NPError NP_GetEntryPoints(NPPluginFuncs* pFuncs)
{
  printf("NP_GetEntryPoints()\n");
  return 0;
}

NPError NP_Initialize(NPNetscapeFuncs* pFuncs)
{
  printf("NP_Initialize()\n");
  return 0;
}

NPError NP_Shutdown()
{
  return 0;
}
