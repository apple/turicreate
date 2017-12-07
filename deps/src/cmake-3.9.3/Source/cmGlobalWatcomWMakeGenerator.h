/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalWatcomWMakeGenerator_h
#define cmGlobalWatcomWMakeGenerator_h

#include "cmConfigure.h"

#include "cmGlobalGeneratorFactory.h"
#include "cmGlobalUnixMakefileGenerator3.h"

#include <string>
#include <vector>

class cmMakefile;
class cmake;
struct cmDocumentationEntry;

/** \class cmGlobalWatcomWMakeGenerator
 * \brief Write a NMake makefiles.
 *
 * cmGlobalWatcomWMakeGenerator manages nmake build process for a tree
 */
class cmGlobalWatcomWMakeGenerator : public cmGlobalUnixMakefileGenerator3
{
public:
  cmGlobalWatcomWMakeGenerator(cmake* cm);
  static cmGlobalGeneratorFactory* NewFactory()
  {
    return new cmGlobalGeneratorSimpleFactory<cmGlobalWatcomWMakeGenerator>();
  }
  ///! Get the name for the generator.
  std::string GetName() const CM_OVERRIDE
  {
    return cmGlobalWatcomWMakeGenerator::GetActualName();
  }
  static std::string GetActualName() { return "Watcom WMake"; }

  /** Get the documentation entry for this generator.  */
  static void GetDocumentation(cmDocumentationEntry& entry);

  /**
   * Try to determine system information such as shared library
   * extension, pthreads, byte order etc.
   */
  void EnableLanguage(std::vector<std::string> const& languages, cmMakefile*,
                      bool optional) CM_OVERRIDE;

  bool AllowNotParallel() const CM_OVERRIDE { return false; }
  bool AllowDeleteOnError() const CM_OVERRIDE { return false; }
};

#endif
