/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalNMakeMakefileGenerator_h
#define cmGlobalNMakeMakefileGenerator_h

#include "cmGlobalUnixMakefileGenerator3.h"

/** \class cmGlobalNMakeMakefileGenerator
 * \brief Write a NMake makefiles.
 *
 * cmGlobalNMakeMakefileGenerator manages nmake build process for a tree
 */
class cmGlobalNMakeMakefileGenerator : public cmGlobalUnixMakefileGenerator3
{
public:
  cmGlobalNMakeMakefileGenerator(cmake* cm);
  static cmGlobalGeneratorFactory* NewFactory()
  {
    return new cmGlobalGeneratorSimpleFactory<
      cmGlobalNMakeMakefileGenerator>();
  }
  ///! Get the name for the generator.
  virtual std::string GetName() const
  {
    return cmGlobalNMakeMakefileGenerator::GetActualName();
  }
  static std::string GetActualName() { return "NMake Makefiles"; }

  /** Get encoding used by generator for makefile files */
  codecvt::Encoding GetMakefileEncoding() const CM_OVERRIDE
  {
    return codecvt::ANSI;
  }

  /** Get the documentation entry for this generator.  */
  static void GetDocumentation(cmDocumentationEntry& entry);

  /**
   * Try to determine system information such as shared library
   * extension, pthreads, byte order etc.
   */
  virtual void EnableLanguage(std::vector<std::string> const& languages,
                              cmMakefile*, bool optional);

private:
  void PrintCompilerAdvice(std::ostream& os, std::string const& lang,
                           const char* envVar) const;
};

#endif
