/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmLocalGhsMultiGenerator_h
#define cmLocalGhsMultiGenerator_h

#include "cmLocalGenerator.h"

class cmGeneratedFileStream;

/** \class cmLocalGhsMultiGenerator
 * \brief Write Green Hills MULTI project files.
 *
 * cmLocalGhsMultiGenerator produces a set of .gpj
 * file for each target in its mirrored directory.
 */
class cmLocalGhsMultiGenerator : public cmLocalGenerator
{
public:
  cmLocalGhsMultiGenerator(cmGlobalGenerator* gg, cmMakefile* mf);

  virtual ~cmLocalGhsMultiGenerator();

  /**
   * Generate the makefile for this directory.
   */
  virtual void Generate();
};

#endif
