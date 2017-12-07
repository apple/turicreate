/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmLocalVisualStudio10Generator_h
#define cmLocalVisualStudio10Generator_h

#include "cmConfigure.h"

#include <string>

#include "cmLocalVisualStudio7Generator.h"

class cmGlobalGenerator;
class cmMakefile;

/** \class cmLocalVisualStudio10Generator
 * \brief Write Visual Studio 10 project files.
 *
 * cmLocalVisualStudio10Generator produces a Visual Studio 10 project
 * file for each target in its directory.
 */
class cmLocalVisualStudio10Generator : public cmLocalVisualStudio7Generator
{
public:
  ///! Set cache only and recurse to false by default.
  cmLocalVisualStudio10Generator(cmGlobalGenerator* gg, cmMakefile* mf);

  virtual ~cmLocalVisualStudio10Generator();

  /**
   * Generate the makefile for this directory.
   */
  virtual void Generate();
  virtual void ReadAndStoreExternalGUID(const std::string& name,
                                        const char* path);

protected:
  virtual const char* ReportErrorLabel() const;
  virtual bool CustomCommandUseLocal() const { return true; }

private:
};
#endif
