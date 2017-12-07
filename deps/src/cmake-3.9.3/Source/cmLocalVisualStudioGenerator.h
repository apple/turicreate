/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmLocalVisualStudioGenerator_h
#define cmLocalVisualStudioGenerator_h

#include "cmConfigure.h"

#include <map>
#include <memory>
#include <string>

#include "cmGlobalVisualStudioGenerator.h"
#include "cmLocalGenerator.h"
#include "cm_auto_ptr.hxx"

class cmCustomCommand;
class cmCustomCommandGenerator;
class cmGeneratorTarget;
class cmGlobalGenerator;
class cmMakefile;
class cmSourceFile;

/** \class cmLocalVisualStudioGenerator
 * \brief Base class for Visual Studio generators.
 *
 * cmLocalVisualStudioGenerator provides functionality common to all
 * Visual Studio generators.
 */
class cmLocalVisualStudioGenerator : public cmLocalGenerator
{
public:
  cmLocalVisualStudioGenerator(cmGlobalGenerator* gg, cmMakefile* mf);
  virtual ~cmLocalVisualStudioGenerator();

  /** Construct a script from the given list of command lines.  */
  std::string ConstructScript(cmCustomCommandGenerator const& ccg,
                              const std::string& newline = "\n");

  /** Label to which to jump in a batch file after a failed step in a
      sequence of custom commands. */
  const char* GetReportErrorLabel() const;

  cmGlobalVisualStudioGenerator::VSVersion GetVersion() const;

  virtual std::string ComputeLongestObjectDirectory(
    cmGeneratorTarget const*) const = 0;

  virtual void AddCMakeListsRules() = 0;

  virtual void ComputeObjectFilenames(
    std::map<cmSourceFile const*, std::string>& mapping,
    cmGeneratorTarget const* = 0);

protected:
  virtual const char* ReportErrorLabel() const;
  virtual bool CustomCommandUseLocal() const { return false; }

  /** Construct a custom command to make exe import lib dir.  */
  CM_AUTO_PTR<cmCustomCommand> MaybeCreateImplibDir(cmGeneratorTarget* target,
                                                    const std::string& config,
                                                    bool isFortran);
};

#endif
