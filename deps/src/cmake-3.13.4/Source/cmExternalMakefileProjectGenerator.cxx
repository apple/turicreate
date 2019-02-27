/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmExternalMakefileProjectGenerator.h"

class cmMakefile;

void cmExternalMakefileProjectGenerator::EnableLanguage(
  std::vector<std::string> const& /*unused*/, cmMakefile* /*unused*/,
  bool /*unused*/)
{
}

std::string cmExternalMakefileProjectGenerator::CreateFullGeneratorName(
  const std::string& globalGenerator, const std::string& extraGenerator)
{
  std::string fullName;
  if (!globalGenerator.empty()) {
    if (!extraGenerator.empty()) {
      fullName = extraGenerator;
      fullName += " - ";
    }
    fullName += globalGenerator;
  }
  return fullName;
}

bool cmExternalMakefileProjectGenerator::Open(
  const std::string& /*bindir*/, const std::string& /*projectName*/,
  bool /*dryRun*/)
{
  return false;
}

cmExternalMakefileProjectGeneratorFactory::
  cmExternalMakefileProjectGeneratorFactory(const std::string& n,
                                            const std::string& doc)
  : Name(n)
  , Documentation(doc)
{
}

cmExternalMakefileProjectGeneratorFactory::
  ~cmExternalMakefileProjectGeneratorFactory()
{
}

std::string cmExternalMakefileProjectGeneratorFactory::GetName() const
{
  return this->Name;
}

std::string cmExternalMakefileProjectGeneratorFactory::GetDocumentation() const
{
  return this->Documentation;
}

std::vector<std::string>
cmExternalMakefileProjectGeneratorFactory::GetSupportedGlobalGenerators() const
{
  return this->SupportedGlobalGenerators;
}

void cmExternalMakefileProjectGeneratorFactory::AddSupportedGlobalGenerator(
  const std::string& base)
{
  this->SupportedGlobalGenerators.push_back(base);
}
