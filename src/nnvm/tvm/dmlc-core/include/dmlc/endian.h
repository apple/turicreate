/*!
 *  Copyright (c) 2017 by Contributors
 * \file endian.h
 * \brief Endian testing
 */
#ifndef DMLC_ENDIAN_H_
#define DMLC_ENDIAN_H_

#if defined(__APPLE__) || defined(_WIN32)
#define DMLC_LITTLE_ENDIAN 1
#else
#include <endian.h>
#define DMLC_LITTLE_ENDIAN (__BYTE_ORDER == __LITTLE_ENDIAN)
#endif

#endif  // DMLC_ENDIAN_H_

