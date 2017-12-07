/* ustd.h common macros and includes */
#ifndef LIBRHASH_USTD_H
#define LIBRHASH_USTD_H

/* Include KWSys Large File Support configuration. */
#include <cmsys/Configure.h>

#if defined(_MSC_VER)
# pragma warning(push,1)
#endif

#include <cm_kwiml.h>

#ifndef KWIML_INT_HAVE_INT64_T
# define int64_t KWIML_INT_int64_t
#endif
#ifndef KWIML_INT_HAVE_INT32_T
# define int32_t KWIML_INT_int32_t
#endif
#ifndef KWIML_INT_HAVE_INT16_T
# define int16_t KWIML_INT_int16_t
#endif
#ifndef KWIML_INT_HAVE_INT8_T
# define int8_t KWIML_INT_int8_t
#endif
#ifndef KWIML_INT_HAVE_UINT64_T
# define uint64_t KWIML_INT_uint64_t
#endif
#ifndef KWIML_INT_HAVE_UINT32_T
# define uint32_t KWIML_INT_uint32_t
#endif
#ifndef KWIML_INT_HAVE_UINT16_T
# define uint16_t KWIML_INT_uint16_t
#endif
#ifndef KWIML_INT_HAVE_UINT8_T
# define uint8_t KWIML_INT_uint8_t
#endif

#endif /* LIBRHASH_USTD_H */
