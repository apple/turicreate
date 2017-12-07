#include "libshared.h"

int libshared::Class::method() const
{
  return 0;
}

int libshared::Class::method_exported() const
{
  return 0;
}

int libshared::Class::method_deprecated() const
{
  return 0;
}

int libshared::Class::method_deprecated_exported() const
{
  return 0;
}

int libshared::Class::method_excluded() const
{
  return 0;
}

int const libshared::Class::data = 1;

int const libshared::Class::data_exported = 1;

int const libshared::Class::data_excluded = 1;

int libshared::ExportedClass::method() const
{
  return 0;
}

int libshared::ExportedClass::method_deprecated() const
{
  return 0;
}

int libshared::ExportedClass::method_excluded() const
{
  return 0;
}

int const libshared::ExportedClass::data = 1;

int const libshared::ExportedClass::data_excluded = 1;

int libshared::ExcludedClass::method() const
{
  return 0;
}

int libshared::ExcludedClass::method_exported() const
{
  return 0;
}

int libshared::ExcludedClass::method_deprecated() const
{
  return 0;
}

int libshared::ExcludedClass::method_deprecated_exported() const
{
  return 0;
}

int libshared::ExcludedClass::method_excluded() const
{
  return 0;
}

int const libshared::ExcludedClass::data = 1;

int const libshared::ExcludedClass::data_exported = 1;

int const libshared::ExcludedClass::data_excluded = 1;

int libshared::function()
{
  return 0;
}

int libshared::function_exported()
{
  return 0;
}

int libshared::function_deprecated()
{
  return 0;
}

int libshared::function_deprecated_exported()
{
  return 0;
}

int libshared::function_excluded()
{
  return 0;
}

int const libshared::data = 1;

int const libshared::data_exported = 1;

int const libshared::data_excluded = 1;

void use_int(int)
{
}
