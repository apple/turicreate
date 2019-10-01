/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalMSYSMakefileGenerator_h
#define cmGlobalMSYSMakefileGenerator_h

#include "cmGlobalUnixMakefileGenerator3.h"

/** \class cmGlobalMSYSMakefileGenerator
 * \brief Write a NMake makefiles.
 *
 * cmGlobalMSYSMakefileGenerator manages nmake build process for a tree
 */
class cmGlobalMSYSMakefileGenerator : public cmGlobalUnixMakefileGenerator3
{
public:
  cmGlobalMSYSMakefileGenerator(cmake* cm);
  static cmGlobalGeneratorFactory* NewFactory()
  {
    return new cmGlobalGeneratorSimpleFactory<cmGlobalMSYSMakefileGenerator>();
  }

  ///! Get the name for the generator.
  virtual std::string GetName() const
  {
    return cmGlobalMSYSMakefileGenerator::GetActualName();
  }
  static std::string GetActualName() { return "MSYS Makefiles"; }

  /** Get the documentation entry for this generator.  */
  static void GetDocumentation(cmDocumentationEntry& entry);

  /**
   * Try to determine system information such as shared library
   * extension, pthreads, byte order etc.
   */
  virtual void EnableLanguage(std::vector<std::string> const& languages,
                              cmMakefile*, bool optional);

private:
  std::string FindMinGW(std::string const& makeloc);
};

#endif
