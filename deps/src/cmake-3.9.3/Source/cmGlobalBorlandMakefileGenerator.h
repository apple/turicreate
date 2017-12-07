/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalBorlandMakefileGenerator_h
#define cmGlobalBorlandMakefileGenerator_h

#include "cmGlobalNMakeMakefileGenerator.h"

/** \class cmGlobalBorlandMakefileGenerator
 * \brief Write a Borland makefiles.
 *
 * cmGlobalBorlandMakefileGenerator manages nmake build process for a tree
 */
class cmGlobalBorlandMakefileGenerator : public cmGlobalUnixMakefileGenerator3
{
public:
  cmGlobalBorlandMakefileGenerator(cmake* cm);
  static cmGlobalGeneratorFactory* NewFactory()
  {
    return new cmGlobalGeneratorSimpleFactory<
      cmGlobalBorlandMakefileGenerator>();
  }

  ///! Get the name for the generator.
  virtual std::string GetName() const
  {
    return cmGlobalBorlandMakefileGenerator::GetActualName();
  }
  static std::string GetActualName() { return "Borland Makefiles"; }

  /** Get the documentation entry for this generator.  */
  static void GetDocumentation(cmDocumentationEntry& entry);

  ///! Create a local generator appropriate to this Global Generator
  virtual cmLocalGenerator* CreateLocalGenerator(cmMakefile* mf);

  /**
   * Try to determine system information such as shared library
   * extension, pthreads, byte order etc.
   */
  virtual void EnableLanguage(std::vector<std::string> const& languages,
                              cmMakefile*, bool optional);

  virtual bool AllowNotParallel() const { return false; }
  virtual bool AllowDeleteOnError() const { return false; }
};

#endif
