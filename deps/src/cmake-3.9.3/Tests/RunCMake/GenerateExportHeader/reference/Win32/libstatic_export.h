
#ifndef LIBSTATIC_EXPORT_H
#define LIBSTATIC_EXPORT_H

#ifdef LIBSTATIC_STATIC_DEFINE
#  define LIBSTATIC_EXPORT
#  define LIBSTATIC_NO_EXPORT
#else
#  ifndef LIBSTATIC_EXPORT
#    ifdef libstatic_EXPORTS
        /* We are building this library */
#      define LIBSTATIC_EXPORT
#    else
        /* We are using this library */
#      define LIBSTATIC_EXPORT
#    endif
#  endif

#  ifndef LIBSTATIC_NO_EXPORT
#    define LIBSTATIC_NO_EXPORT
#  endif
#endif

#ifndef LIBSTATIC_DEPRECATED
#  define LIBSTATIC_DEPRECATED __declspec(deprecated)
#endif

#ifndef LIBSTATIC_DEPRECATED_EXPORT
#  define LIBSTATIC_DEPRECATED_EXPORT LIBSTATIC_EXPORT LIBSTATIC_DEPRECATED
#endif

#ifndef LIBSTATIC_DEPRECATED_NO_EXPORT
#  define LIBSTATIC_DEPRECATED_NO_EXPORT LIBSTATIC_NO_EXPORT LIBSTATIC_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef LIBSTATIC_NO_DEPRECATED
#    define LIBSTATIC_NO_DEPRECATED
#  endif
#endif

#endif
