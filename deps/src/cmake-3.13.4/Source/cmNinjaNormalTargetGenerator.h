/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmNinjaNormalTargetGenerator_h
#define cmNinjaNormalTargetGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmNinjaTargetGenerator.h"

#include <string>
#include <vector>

class cmGeneratorTarget;

class cmNinjaNormalTargetGenerator : public cmNinjaTargetGenerator
{
public:
  cmNinjaNormalTargetGenerator(cmGeneratorTarget* target);
  ~cmNinjaNormalTargetGenerator() override;

  void Generate() override;

private:
  std::string LanguageLinkerRule() const;
  std::string LanguageLinkerDeviceRule() const;

  const char* GetVisibleTypeName() const;
  void WriteLanguagesRules();

  void WriteLinkRule(bool useResponseFile);
  void WriteDeviceLinkRule(bool useResponseFile);

  void WriteLinkStatement();
  void WriteDeviceLinkStatement();

  void WriteObjectLibStatement();

  std::vector<std::string> ComputeLinkCmd();
  std::vector<std::string> ComputeDeviceLinkCmd();

private:
  // Target name info.
  std::string TargetNameOut;
  std::string TargetNameSO;
  std::string TargetNameReal;
  std::string TargetNameImport;
  std::string TargetNamePDB;
  std::string TargetLinkLanguage;
  std::string DeviceLinkObject;
};

#endif // ! cmNinjaNormalTargetGenerator_h
