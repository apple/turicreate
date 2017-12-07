
#ifndef LIBSHARED_EXPORT_H
#define LIBSHARED_EXPORT_H

#ifdef LIBSHARED_STATIC_DEFINE
#  define LIBSHARED_EXPORT
#  define LIBSHARED_NO_EXPORT
#else
#  ifndef LIBSHARED_EXPORT
#    ifdef libshared_EXPORTS
        /* We are building this library */
#      define LIBSHARED_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define LIBSHARED_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef LIBSHARED_NO_EXPORT
#    define LIBSHARED_NO_EXPORT
#  endif
#endif

#ifndef LIBSHARED_DEPRECATED
#  define LIBSHARED_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef LIBSHARED_DEPRECATED_EXPORT
#  define LIBSHARED_DEPRECATED_EXPORT LIBSHARED_EXPORT LIBSHARED_DEPRECATED
#endif

#ifndef LIBSHARED_DEPRECATED_NO_EXPORT
#  define LIBSHARED_DEPRECATED_NO_EXPORT LIBSHARED_NO_EXPORT LIBSHARED_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef LIBSHARED_NO_DEPRECATED
#    define LIBSHARED_NO_DEPRECATED
#  endif
#endif

#endif
