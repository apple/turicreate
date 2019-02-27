#ifndef LIBSTATIC_H
#define LIBSTATIC_H

#include "libstatic_export.h"

namespace libstatic {

class Class
{
public:
  int method() const;

  int LIBSTATIC_EXPORT method_exported() const;

  int LIBSTATIC_DEPRECATED method_deprecated() const;

  int LIBSTATIC_DEPRECATED_EXPORT method_deprecated_exported() const;

  int LIBSTATIC_NO_EXPORT method_excluded() const;

  static int const data;

  static int const LIBSTATIC_EXPORT data_exported;

  static int const LIBSTATIC_NO_EXPORT data_excluded;
};

class LIBSTATIC_EXPORT ExportedClass
{
public:
  int method() const;

  int LIBSTATIC_EXPORT method_exported() const;

  int LIBSTATIC_DEPRECATED method_deprecated() const;

  int LIBSTATIC_DEPRECATED_EXPORT method_deprecated_exported() const;

  int LIBSTATIC_NO_EXPORT method_excluded() const;

  static int const data;

  static int const LIBSTATIC_EXPORT data_exported;

  static int const LIBSTATIC_NO_EXPORT data_excluded;
};

class LIBSTATIC_NO_EXPORT ExcludedClass
{
public:
  int method() const;

  int LIBSTATIC_EXPORT method_exported() const;

  int LIBSTATIC_DEPRECATED method_deprecated() const;

  int LIBSTATIC_DEPRECATED_EXPORT method_deprecated_exported() const;

  int LIBSTATIC_NO_EXPORT method_excluded() const;

  static int const data;

  static int const LIBSTATIC_EXPORT data_exported;

  static int const LIBSTATIC_NO_EXPORT data_excluded;
};

int function();

int LIBSTATIC_EXPORT function_exported();

int LIBSTATIC_DEPRECATED function_deprecated();

int LIBSTATIC_DEPRECATED_EXPORT function_deprecated_exported();

int LIBSTATIC_NO_EXPORT function_excluded();

extern int const data;

extern int const LIBSTATIC_EXPORT data_exported;

extern int const LIBSTATIC_NO_EXPORT data_excluded;

} // namespace libstatic

#endif
