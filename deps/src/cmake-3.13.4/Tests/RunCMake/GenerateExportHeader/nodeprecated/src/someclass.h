
#include "nodeprecatedlib_export.h"

class NODEPRECATEDLIB_EXPORT SomeClass
{
public:
#ifndef NODEPRECATEDLIB_NO_DEPRECATED
  void someMethod() const;
#endif
};
