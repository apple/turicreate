/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmLocalXCodeGenerator.h"

#include "cmGeneratorTarget.h"
#include "cmGlobalXCodeGenerator.h"
#include "cmSourceFile.h"

class cmGeneratorTarget;
class cmGlobalGenerator;
class cmMakefile;

cmLocalXCodeGenerator::cmLocalXCodeGenerator(cmGlobalGenerator* gg,
                                             cmMakefile* mf)
  : cmLocalGenerator(gg, mf)
{
  // the global generator does this, so do not
  // put these flags into the language flags
  this->EmitUniversalBinaryFlags = false;
}

cmLocalXCodeGenerator::~cmLocalXCodeGenerator()
{
}

std::string cmLocalXCodeGenerator::GetTargetDirectory(
  cmGeneratorTarget const*) const
{
  // No per-target directory for this generator (yet).
  return "";
}

void cmLocalXCodeGenerator::AppendFlagEscape(std::string& flags,
                                             const std::string& rawFlag)
{
  cmGlobalXCodeGenerator* gg =
    static_cast<cmGlobalXCodeGenerator*>(this->GlobalGenerator);
  gg->AppendFlag(flags, rawFlag);
}

void cmLocalXCodeGenerator::Generate()
{
  cmLocalGenerator::Generate();

  std::vector<cmGeneratorTarget*> targets = this->GetGeneratorTargets();
  for (std::vector<cmGeneratorTarget*>::iterator iter = targets.begin();
       iter != targets.end(); ++iter) {
    (*iter)->HasMacOSXRpathInstallNameDir("");
  }
}

void cmLocalXCodeGenerator::GenerateInstallRules()
{
  cmLocalGenerator::GenerateInstallRules();

  std::vector<cmGeneratorTarget*> targets = this->GetGeneratorTargets();
  for (std::vector<cmGeneratorTarget*>::iterator iter = targets.begin();
       iter != targets.end(); ++iter) {
    (*iter)->HasMacOSXRpathInstallNameDir("");
  }
}

void cmLocalXCodeGenerator::ComputeObjectFilenames(
  std::map<cmSourceFile const*, std::string>& mapping,
  cmGeneratorTarget const*)
{
  // Count the number of object files with each name. Warn about duplicate
  // names since Xcode names them uniquely automatically with a numeric suffix
  // to avoid exact duplicate file names. Note that Mac file names are not
  // typically case sensitive, hence the LowerCase.
  std::map<std::string, int> counts;
  for (std::map<cmSourceFile const*, std::string>::iterator si =
         mapping.begin();
       si != mapping.end(); ++si) {
    cmSourceFile const* sf = si->first;
    std::string objectName =
      cmSystemTools::GetFilenameWithoutLastExtension(sf->GetFullPath());
    objectName += ".o";

    std::string objectNameLower = cmSystemTools::LowerCase(objectName);
    counts[objectNameLower] += 1;
    if (2 == counts[objectNameLower]) {
      // TODO: emit warning about duplicate name?
    }
    si->second = objectName;
  }
}
