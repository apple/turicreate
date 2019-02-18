/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmQtAutoGenInitializer_h
#define cmQtAutoGenInitializer_h

#include "cmConfigure.h" // IWYU pragma: keep
#include "cmQtAutoGen.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class cmGeneratorTarget;
class cmTarget;

/// @brief Initializes the QtAutoGen generators
class cmQtAutoGenInitializer : public cmQtAutoGen
{
public:
  /// @brief Rcc job information
  class Qrc
  {
  public:
    Qrc()
      : Generated(false)
      , Unique(false)
    {
    }

  public:
    std::string LockFile;
    std::string QrcFile;
    std::string QrcName;
    std::string PathChecksum;
    std::string InfoFile;
    std::string SettingsFile;
    std::map<std::string, std::string> ConfigSettingsFile;
    std::string RccFile;
    bool Generated;
    bool Unique;
    std::vector<std::string> Options;
    std::vector<std::string> Resources;
  };

public:
  static IntegerVersion GetQtVersion(cmGeneratorTarget const* target);

  cmQtAutoGenInitializer(cmGeneratorTarget* target, bool mocEnabled,
                         bool uicEnabled, bool rccEnabled,
                         IntegerVersion const& qtVersion);

  bool InitCustomTargets();
  bool SetupCustomTargets();

private:
  bool InitMoc();
  bool InitUic();
  bool InitRcc();

  bool InitScanFiles();
  bool InitAutogenTarget();
  bool InitRccTargets();

  bool SetupWriteAutogenInfo();
  bool SetupWriteRccInfo();

  void AddGeneratedSource(std::string const& filename, GeneratorT genType);

  bool GetMocExecutable();
  bool GetUicExecutable();
  bool GetRccExecutable();

  bool RccListInputs(std::string const& fileName,
                     std::vector<std::string>& files,
                     std::string& errorMessage);

private:
  cmGeneratorTarget* Target;

  // Configuration
  IntegerVersion QtVersion;
  bool MultiConfig = false;
  std::string ConfigDefault;
  std::vector<std::string> ConfigsList;
  std::string Verbosity;
  std::string TargetsFolder;

  /// @brief Common directories
  struct
  {
    std::string Info;
    std::string Build;
    std::string Work;
    std::string Include;
    std::map<std::string, std::string> ConfigInclude;
  } Dir;

  /// @brief Autogen target variables
  struct
  {
    std::string Name;
    // Settings
    std::string Parallel;
    // Configuration files
    std::string InfoFile;
    std::string SettingsFile;
    std::map<std::string, std::string> ConfigSettingsFile;
    // Dependencies
    std::set<std::string> DependFiles;
    std::set<cmTarget*> DependTargets;
    // Sources to process
    std::vector<std::string> Headers;
    std::vector<std::string> Sources;
    std::vector<std::string> HeadersGenerated;
    std::vector<std::string> SourcesGenerated;
  } AutogenTarget;

  /// @brief Moc only variables
  struct
  {
    bool Enabled = false;
    std::string Executable;
    std::string PredefsCmd;
    std::set<std::string> Skip;
    std::string Includes;
    std::map<std::string, std::string> ConfigIncludes;
    std::string Defines;
    std::map<std::string, std::string> ConfigDefines;
    std::string MocsCompilation;
  } Moc;

  ///@brief Uic only variables
  struct
  {
    bool Enabled = false;
    std::string Executable;
    std::set<std::string> Skip;
    std::vector<std::string> SearchPaths;
    std::string Options;
    std::map<std::string, std::string> ConfigOptions;
    std::vector<std::string> FileFiles;
    std::vector<std::vector<std::string>> FileOptions;
  } Uic;

  /// @brief Rcc only variables
  struct
  {
    bool Enabled = false;
    std::string Executable;
    std::vector<std::string> ListOptions;
    std::vector<Qrc> Qrcs;
  } Rcc;
};

#endif
