/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalCommonGenerator_h
#define cmGlobalCommonGenerator_h

#include "cmConfigure.h"

#include "cmGlobalGenerator.h"

class cmake;

/** \class cmGlobalCommonGenerator
 * \brief Common infrastructure for Makefile and Ninja global generators.
 */
class cmGlobalCommonGenerator : public cmGlobalGenerator
{
public:
  cmGlobalCommonGenerator(cmake* cm);
  ~cmGlobalCommonGenerator() CM_OVERRIDE;
};

#endif
