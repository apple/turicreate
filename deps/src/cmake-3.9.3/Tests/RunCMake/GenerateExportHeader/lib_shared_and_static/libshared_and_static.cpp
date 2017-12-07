#include "libshared_and_static.h"

#ifndef MY_CUSTOM_CONTENT_ADDED
#error "MY_CUSTOM_CONTENT_ADDED not defined!"
#endif

int libshared_and_static::Class::method() const
{
  return 0;
}

int libshared_and_static::Class::method_exported() const
{
  return 0;
}

int libshared_and_static::Class::method_deprecated() const
{
  return 0;
}

int libshared_and_static::Class::method_deprecated_exported() const
{
  return 0;
}

int libshared_and_static::Class::method_excluded() const
{
  return 0;
}

int const libshared_and_static::Class::data = 1;

int const libshared_and_static::Class::data_exported = 1;

int const libshared_and_static::Class::data_excluded = 1;

int libshared_and_static::ExportedClass::method() const
{
  return 0;
}

int libshared_and_static::ExportedClass::method_deprecated() const
{
  return 0;
}

int libshared_and_static::ExportedClass::method_excluded() const
{
  return 0;
}

int const libshared_and_static::ExportedClass::data = 1;

int const libshared_and_static::ExportedClass::data_excluded = 1;

int libshared_and_static::ExcludedClass::method() const
{
  return 0;
}

int libshared_and_static::ExcludedClass::method_exported() const
{
  return 0;
}

int libshared_and_static::ExcludedClass::method_deprecated() const
{
  return 0;
}

int libshared_and_static::ExcludedClass::method_deprecated_exported() const
{
  return 0;
}

int libshared_and_static::ExcludedClass::method_excluded() const
{
  return 0;
}

int const libshared_and_static::ExcludedClass::data = 1;

int const libshared_and_static::ExcludedClass::data_exported = 1;

int const libshared_and_static::ExcludedClass::data_excluded = 1;

int libshared_and_static::function()
{
  return 0;
}

int libshared_and_static::function_exported()
{
  return 0;
}

int libshared_and_static::function_deprecated()
{
  return 0;
}

int libshared_and_static::function_deprecated_exported()
{
  return 0;
}

int libshared_and_static::function_excluded()
{
  return 0;
}

int const libshared_and_static::data = 1;

int const libshared_and_static::data_exported = 1;

int const libshared_and_static::data_excluded = 1;

void use_int(int)
{
}
