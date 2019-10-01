#include "libstatic.h"

int libstatic::Class::method() const
{
  return 0;
}

int libstatic::Class::method_exported() const
{
  return 0;
}

int libstatic::Class::method_deprecated() const
{
  return 0;
}

int libstatic::Class::method_deprecated_exported() const
{
  return 0;
}

int libstatic::Class::method_excluded() const
{
  return 0;
}

int const libstatic::Class::data = 1;

int const libstatic::Class::data_exported = 1;

int const libstatic::Class::data_excluded = 1;

int libstatic::ExportedClass::method() const
{
  return 0;
}

int libstatic::ExportedClass::method_exported() const
{
  return 0;
}

int libstatic::ExportedClass::method_deprecated() const
{
  return 0;
}

int libstatic::ExportedClass::method_deprecated_exported() const
{
  return 0;
}

int libstatic::ExportedClass::method_excluded() const
{
  return 0;
}

int const libstatic::ExportedClass::data = 1;

int const libstatic::ExportedClass::data_exported = 1;

int const libstatic::ExportedClass::data_excluded = 1;

int libstatic::ExcludedClass::method() const
{
  return 0;
}

int libstatic::ExcludedClass::method_exported() const
{
  return 0;
}

int libstatic::ExcludedClass::method_deprecated() const
{
  return 0;
}

int libstatic::ExcludedClass::method_deprecated_exported() const
{
  return 0;
}

int libstatic::ExcludedClass::method_excluded() const
{
  return 0;
}

int const libstatic::ExcludedClass::data = 1;

int const libstatic::ExcludedClass::data_exported = 1;

int const libstatic::ExcludedClass::data_excluded = 1;

int libstatic::function()
{
  return 0;
}

int libstatic::function_exported()
{
  return 0;
}

int libstatic::function_deprecated()
{
  return 0;
}

int libstatic::function_deprecated_exported()
{
  return 0;
}

int libstatic::function_excluded()
{
  return 0;
}

int const libstatic::data = 1;

int const libstatic::data_exported = 1;

int const libstatic::data_excluded = 1;
