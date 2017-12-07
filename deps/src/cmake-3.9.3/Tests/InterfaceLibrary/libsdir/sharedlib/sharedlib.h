
#ifndef SHAREDLIB_H
#define SHAREDLIB_H

#include "sharedlib_export.h"

#include "shareddependlib.h"

struct SHAREDLIB_EXPORT SharedLibObject
{
  SharedDependLibObject object() const;
  int foo() const;
};

#endif
