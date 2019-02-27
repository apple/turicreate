/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackExternalGenerator_h
#define cmCPackExternalGenerator_h

#include "cmCPackGenerator.h"
#include "cm_sys_stat.h"

#include <memory>
#include <string>

class cmGlobalGenerator;
namespace Json {
class Value;
}

/** \class cmCPackExternalGenerator
 * \brief A generator for CPack External packaging tools
 */
class cmCPackExternalGenerator : public cmCPackGenerator
{
public:
  cmCPackTypeMacro(cmCPackExternalGenerator, cmCPackGenerator);

  const char* GetOutputExtension() override { return ".json"; }

protected:
  int InitializeInternal() override;

  int PackageFiles() override;

  bool SupportsComponentInstallation() const override;

  int InstallProjectViaInstallCommands(
    bool setDestDir, const std::string& tempInstallDirectory) override;
  int InstallProjectViaInstallScript(
    bool setDestDir, const std::string& tempInstallDirectory) override;
  int InstallProjectViaInstalledDirectories(
    bool setDestDir, const std::string& tempInstallDirectory,
    const mode_t* default_dir_mode) override;

  int RunPreinstallTarget(const std::string& installProjectName,
                          const std::string& installDirectory,
                          cmGlobalGenerator* globalGenerator,
                          const std::string& buildConfig) override;
  int InstallCMakeProject(bool setDestDir, const std::string& installDirectory,
                          const std::string& baseTempInstallDirectory,
                          const mode_t* default_dir_mode,
                          const std::string& component, bool componentInstall,
                          const std::string& installSubDirectory,
                          const std::string& buildConfig,
                          std::string& absoluteDestFiles) override;

private:
  bool StagingEnabled() const;

  class cmCPackExternalVersionGenerator
  {
  public:
    cmCPackExternalVersionGenerator(cmCPackExternalGenerator* parent);

    virtual ~cmCPackExternalVersionGenerator() = default;

    virtual int WriteToJSON(Json::Value& root);

  protected:
    virtual int GetVersionMajor() = 0;
    virtual int GetVersionMinor() = 0;

    int WriteVersion(Json::Value& root);

    cmCPackExternalGenerator* Parent;
  };

  class cmCPackExternalVersion1Generator
    : public cmCPackExternalVersionGenerator
  {
  public:
    using cmCPackExternalVersionGenerator::cmCPackExternalVersionGenerator;

  protected:
    int GetVersionMajor() override { return 1; }
    int GetVersionMinor() override { return 0; }
  };

  std::unique_ptr<cmCPackExternalVersionGenerator> Generator;
};

#endif
