/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmLocalCommonGenerator.h"

#include <utility>
#include <vector>

#include "cmGeneratorTarget.h"
#include "cmMakefile.h"
#include "cmOutputConverter.h"

class cmGlobalGenerator;

cmLocalCommonGenerator::cmLocalCommonGenerator(cmGlobalGenerator* gg,
                                               cmMakefile* mf,
                                               std::string const& wd)
  : cmLocalGenerator(gg, mf)
  , WorkingDirectory(wd)
{
  // Store the configuration name that will be generated.
  if (const char* config = this->Makefile->GetDefinition("CMAKE_BUILD_TYPE")) {
    // Use the build type given by the user.
    this->ConfigName = config;
  } else {
    // No configuration type given.
    this->ConfigName.clear();
  }
}

cmLocalCommonGenerator::~cmLocalCommonGenerator()
{
}

std::string cmLocalCommonGenerator::GetTargetFortranFlags(
  cmGeneratorTarget const* target, std::string const& config)
{
  std::string flags;

  // Enable module output if necessary.
  if (const char* modout_flag =
        this->Makefile->GetDefinition("CMAKE_Fortran_MODOUT_FLAG")) {
    this->AppendFlags(flags, modout_flag);
  }

  // Add a module output directory flag if necessary.
  std::string mod_dir =
    target->GetFortranModuleDirectory(this->WorkingDirectory);
  if (!mod_dir.empty()) {
    mod_dir = this->ConvertToOutputFormat(
      this->ConvertToRelativePath(this->WorkingDirectory, mod_dir),
      cmOutputConverter::SHELL);
  } else {
    mod_dir =
      this->Makefile->GetSafeDefinition("CMAKE_Fortran_MODDIR_DEFAULT");
  }
  if (!mod_dir.empty()) {
    std::string modflag =
      this->Makefile->GetRequiredDefinition("CMAKE_Fortran_MODDIR_FLAG");
    modflag += mod_dir;
    this->AppendFlags(flags, modflag);
  }

  // If there is a separate module path flag then duplicate the
  // include path with it.  This compiler does not search the include
  // path for modules.
  if (const char* modpath_flag =
        this->Makefile->GetDefinition("CMAKE_Fortran_MODPATH_FLAG")) {
    std::vector<std::string> includes;
    this->GetIncludeDirectories(includes, target, "C", config);
    for (std::string const& id : includes) {
      std::string flg = modpath_flag;
      flg += this->ConvertToOutputFormat(id, cmOutputConverter::SHELL);
      this->AppendFlags(flags, flg);
    }
  }

  return flags;
}

void cmLocalCommonGenerator::ComputeObjectFilenames(
  std::map<cmSourceFile const*, std::string>& mapping,
  cmGeneratorTarget const* gt)
{
  // Determine if these object files should use a custom extension
  char const* custom_ext = gt->GetCustomObjectExtension();
  for (auto& si : mapping) {
    cmSourceFile const* sf = si.first;
    bool keptSourceExtension;
    si.second = this->GetObjectFileNameWithoutTarget(
      *sf, gt->ObjectDirectory, &keptSourceExtension, custom_ext);
  }
}
