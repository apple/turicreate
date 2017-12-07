#include "cmCPackPropertiesGenerator.h"

#include "cmGeneratorExpression.h"
#include "cmInstalledFile.h"
#include "cmOutputConverter.h"

#include <map>
#include <ostream>
#include <utility>

cmCPackPropertiesGenerator::cmCPackPropertiesGenerator(
  cmLocalGenerator* lg, cmInstalledFile const& installedFile,
  std::vector<std::string> const& configurations)
  : cmScriptGenerator("CPACK_BUILD_CONFIG", configurations)
  , LG(lg)
  , InstalledFile(installedFile)
{
  this->ActionsPerConfig = true;
}

void cmCPackPropertiesGenerator::GenerateScriptForConfig(
  std::ostream& os, const std::string& config, Indent indent)
{
  std::string const& expandedFileName =
    this->InstalledFile.GetNameExpression().Evaluate(this->LG, config);

  cmInstalledFile::PropertyMapType const& properties =
    this->InstalledFile.GetProperties();

  for (cmInstalledFile::PropertyMapType::const_iterator i = properties.begin();
       i != properties.end(); ++i) {
    std::string const& name = i->first;
    cmInstalledFile::Property const& property = i->second;

    os << indent << "set_property(INSTALL "
       << cmOutputConverter::EscapeForCMake(expandedFileName) << " PROPERTY "
       << cmOutputConverter::EscapeForCMake(name);

    for (cmInstalledFile::ExpressionVectorType::const_iterator j =
           property.ValueExpressions.begin();
         j != property.ValueExpressions.end(); ++j) {
      std::string value = (*j)->Evaluate(this->LG, config);
      os << " " << cmOutputConverter::EscapeForCMake(value);
    }

    os << ")\n";
  }
}
