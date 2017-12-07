/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalJOMMakefileGenerator_h
#define cmGlobalJOMMakefileGenerator_h

#include "cmGlobalUnixMakefileGenerator3.h"

/** \class cmGlobalJOMMakefileGenerator
 * \brief Write a JOM makefiles.
 *
 * cmGlobalJOMMakefileGenerator manages nmake build process for a tree
 */
class cmGlobalJOMMakefileGenerator : public cmGlobalUnixMakefileGenerator3
{
public:
  cmGlobalJOMMakefileGenerator(cmake* cm);
  static cmGlobalGeneratorFactory* NewFactory()
  {
    return new cmGlobalGeneratorSimpleFactory<cmGlobalJOMMakefileGenerator>();
  }
  ///! Get the name for the generator.
  virtual std::string GetName() const
  {
    return cmGlobalJOMMakefileGenerator::GetActualName();
  }
  // use NMake Makefiles in the name so that scripts/tests that depend on the
  // name NMake Makefiles will work
  static std::string GetActualName() { return "NMake Makefiles JOM"; }

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
