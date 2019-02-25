#ifndef LIBSHARED_AND_STATIC_H
#define LIBSHARED_AND_STATIC_H

#include "libshared_and_static_export.h"

namespace libshared_and_static {

class Class
{
public:
  int method() const;

  int MYPREFIX_LIBSHARED_AND_STATIC_EXPORT method_exported() const;

  int MYPREFIX_LIBSHARED_AND_STATIC_DEPRECATED method_deprecated() const;

  int MYPREFIX_LIBSHARED_AND_STATIC_DEPRECATED_EXPORT
  method_deprecated_exported() const;

  int MYPREFIX_LIBSHARED_AND_STATIC_NO_EXPORT method_excluded() const;

  static int const data;

  static int const MYPREFIX_LIBSHARED_AND_STATIC_EXPORT data_exported;

  static int const MYPREFIX_LIBSHARED_AND_STATIC_NO_EXPORT data_excluded;
};

class MYPREFIX_LIBSHARED_AND_STATIC_EXPORT ExportedClass
{
public:
  int method() const;

  int MYPREFIX_LIBSHARED_AND_STATIC_DEPRECATED method_deprecated() const;

  int MYPREFIX_LIBSHARED_AND_STATIC_NO_EXPORT method_excluded() const;

  static int const data;

  static int const MYPREFIX_LIBSHARED_AND_STATIC_NO_EXPORT data_excluded;
};

class MYPREFIX_LIBSHARED_AND_STATIC_NO_EXPORT ExcludedClass
{
public:
  int method() const;

  int MYPREFIX_LIBSHARED_AND_STATIC_EXPORT method_exported() const;

  int MYPREFIX_LIBSHARED_AND_STATIC_DEPRECATED method_deprecated() const;

  int MYPREFIX_LIBSHARED_AND_STATIC_DEPRECATED_EXPORT
  method_deprecated_exported() const;

  int MYPREFIX_LIBSHARED_AND_STATIC_NO_EXPORT method_excluded() const;

  static int const data;

  static int const MYPREFIX_LIBSHARED_AND_STATIC_EXPORT data_exported;

  static int const MYPREFIX_LIBSHARED_AND_STATIC_NO_EXPORT data_excluded;
};

int function();

int MYPREFIX_LIBSHARED_AND_STATIC_EXPORT function_exported();

int MYPREFIX_LIBSHARED_AND_STATIC_DEPRECATED function_deprecated();

int MYPREFIX_LIBSHARED_AND_STATIC_DEPRECATED_EXPORT
function_deprecated_exported();

int MYPREFIX_LIBSHARED_AND_STATIC_NO_EXPORT function_excluded();

extern int const data;

extern int const MYPREFIX_LIBSHARED_AND_STATIC_EXPORT data_exported;

extern int const MYPREFIX_LIBSHARED_AND_STATIC_NO_EXPORT data_excluded;

} // namespace libshared_and_static

#endif
