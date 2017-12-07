/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmExportTryCompileFileGenerator.h"

#include "cmGeneratorExpression.h"
#include "cmGeneratorExpressionDAGChecker.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cm_auto_ptr.hxx"

#include <map>
#include <utility>

cmExportTryCompileFileGenerator::cmExportTryCompileFileGenerator(
  cmGlobalGenerator* gg, const std::vector<std::string>& targets,
  cmMakefile* mf)
{
  gg->CreateImportedGenerationObjects(mf, targets, this->Exports);
}

bool cmExportTryCompileFileGenerator::GenerateMainFile(std::ostream& os)
{
  std::set<cmGeneratorTarget const*> emitted;
  std::set<cmGeneratorTarget const*> emittedDeps;
  while (!this->Exports.empty()) {
    cmGeneratorTarget const* te = this->Exports.back();
    this->Exports.pop_back();
    if (emitted.insert(te).second) {
      emittedDeps.insert(te);
      this->GenerateImportTargetCode(os, te);

      ImportPropertyMap properties;

#define FIND_TARGETS(PROPERTY)                                                \
  this->FindTargets("INTERFACE_" #PROPERTY, te, emittedDeps);

      CM_FOR_EACH_TRANSITIVE_PROPERTY_NAME(FIND_TARGETS)

#undef FIND_TARGETS

      this->PopulateProperties(te, properties, emittedDeps);

      this->GenerateInterfaceProperties(te, os, properties);
    }
  }
  return true;
}

std::string cmExportTryCompileFileGenerator::FindTargets(
  const std::string& propName, cmGeneratorTarget const* tgt,
  std::set<cmGeneratorTarget const*>& emitted)
{
  const char* prop = tgt->GetProperty(propName);
  if (!prop) {
    return std::string();
  }

  cmGeneratorExpression ge;

  cmGeneratorExpressionDAGChecker dagChecker(tgt->GetName(), propName,
                                             CM_NULLPTR, CM_NULLPTR);

  CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = ge.Parse(prop);

  cmTarget dummyHead("try_compile_dummy_exe", cmStateEnums::EXECUTABLE,
                     cmTarget::VisibilityNormal, tgt->Target->GetMakefile());

  cmGeneratorTarget gDummyHead(&dummyHead, tgt->GetLocalGenerator());

  std::string result = cge->Evaluate(tgt->GetLocalGenerator(), this->Config,
                                     false, &gDummyHead, tgt, &dagChecker);

  const std::set<cmGeneratorTarget const*>& allTargets =
    cge->GetAllTargetsSeen();
  for (std::set<cmGeneratorTarget const*>::const_iterator li =
         allTargets.begin();
       li != allTargets.end(); ++li) {
    if (emitted.insert(*li).second) {
      this->Exports.push_back(*li);
    }
  }
  return result;
}

void cmExportTryCompileFileGenerator::PopulateProperties(
  const cmGeneratorTarget* target, ImportPropertyMap& properties,
  std::set<cmGeneratorTarget const*>& emitted)
{
  std::vector<std::string> props = target->GetPropertyKeys();
  for (std::vector<std::string>::const_iterator i = props.begin();
       i != props.end(); ++i) {

    properties[*i] = target->GetProperty(*i);

    if (i->find("IMPORTED_LINK_INTERFACE_LIBRARIES") == 0 ||
        i->find("IMPORTED_LINK_DEPENDENT_LIBRARIES") == 0 ||
        i->find("INTERFACE_LINK_LIBRARIES") == 0) {
      std::string evalResult = this->FindTargets(*i, target, emitted);

      std::vector<std::string> depends;
      cmSystemTools::ExpandListArgument(evalResult, depends);
      for (std::vector<std::string>::const_iterator li = depends.begin();
           li != depends.end(); ++li) {
        cmGeneratorTarget* tgt =
          target->GetLocalGenerator()->FindGeneratorTargetToUse(*li);
        if (tgt && emitted.insert(tgt).second) {
          this->Exports.push_back(tgt);
        }
      }
    }
  }
}

std::string cmExportTryCompileFileGenerator::InstallNameDir(
  cmGeneratorTarget* target, const std::string& config)
{
  std::string install_name_dir;

  cmMakefile* mf = target->Target->GetMakefile();
  if (mf->IsOn("CMAKE_PLATFORM_HAS_INSTALLNAME")) {
    install_name_dir = target->GetInstallNameDirForBuildTree(config);
  }

  return install_name_dir;
}
