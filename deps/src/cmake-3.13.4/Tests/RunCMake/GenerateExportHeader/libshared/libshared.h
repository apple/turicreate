#ifndef LIBSHARED_H
#define LIBSHARED_H

#include "libshared_export.h"

namespace libshared {

class Class
{
public:
  int method() const;

  int LIBSHARED_EXPORT method_exported() const;

  int LIBSHARED_DEPRECATED method_deprecated() const;

  int LIBSHARED_DEPRECATED_EXPORT method_deprecated_exported() const;

  int LIBSHARED_NO_EXPORT method_excluded() const;

  static int const data;

  static int const LIBSHARED_EXPORT data_exported;

  static int const LIBSHARED_NO_EXPORT data_excluded;
};

class LIBSHARED_EXPORT ExportedClass
{
public:
  int method() const;

  int LIBSHARED_DEPRECATED method_deprecated() const;

  int LIBSHARED_NO_EXPORT method_excluded() const;

  static int const data;

  static int const LIBSHARED_NO_EXPORT data_excluded;
};

class LIBSHARED_NO_EXPORT ExcludedClass
{
public:
  int method() const;

  int LIBSHARED_EXPORT method_exported() const;

  int LIBSHARED_DEPRECATED method_deprecated() const;

  int LIBSHARED_DEPRECATED_EXPORT method_deprecated_exported() const;

  int LIBSHARED_NO_EXPORT method_excluded() const;

  static int const data;

  static int const LIBSHARED_EXPORT data_exported;

  static int const LIBSHARED_NO_EXPORT data_excluded;
};

int function();

int LIBSHARED_EXPORT function_exported();

int LIBSHARED_DEPRECATED function_deprecated();

int LIBSHARED_DEPRECATED_EXPORT function_deprecated_exported();

int LIBSHARED_NO_EXPORT function_excluded();

extern int const data;

extern int const LIBSHARED_EXPORT data_exported;

extern int const LIBSHARED_NO_EXPORT data_excluded;

} // namespace libshared

LIBSHARED_EXPORT void use_int(int);

#endif
