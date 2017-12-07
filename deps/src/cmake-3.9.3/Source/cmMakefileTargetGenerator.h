/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmMakefileTargetGenerator_h
#define cmMakefileTargetGenerator_h

#include "cmConfigure.h"

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "cmCommonTargetGenerator.h"
#include "cmLocalUnixMakefileGenerator3.h"
#include "cmOSXBundleGenerator.h"

class cmCustomCommandGenerator;
class cmGeneratedFileStream;
class cmGeneratorTarget;
class cmGlobalUnixMakefileGenerator3;
class cmLinkLineComputer;
class cmOutputConverter;
class cmSourceFile;
class cmStateDirectory;

/** \class cmMakefileTargetGenerator
 * \brief Support Routines for writing makefiles
 *
 */
class cmMakefileTargetGenerator : public cmCommonTargetGenerator
{
public:
  // constructor to set the ivars
  cmMakefileTargetGenerator(cmGeneratorTarget* target);
  ~cmMakefileTargetGenerator() CM_OVERRIDE;

  // construct using this factory call
  static cmMakefileTargetGenerator* New(cmGeneratorTarget* tgt);

  /* the main entry point for this class. Writes the Makefiles associated
     with this target */
  virtual void WriteRuleFiles() = 0;

  /* return the number of actions that have progress reporting on them */
  virtual unsigned long GetNumberOfProgressActions()
  {
    return this->NumberOfProgressActions;
  }
  std::string GetProgressFileNameFull() { return this->ProgressFileNameFull; }

  cmGeneratorTarget* GetGeneratorTarget() { return this->GeneratorTarget; }

protected:
  // create the file and directory etc
  void CreateRuleFile();

  // outputs the rules for object files and custom commands used by
  // this target
  void WriteTargetBuildRules();

  // write some common code at the top of build.make
  void WriteCommonCodeRules();
  void WriteTargetLanguageFlags();

  // write the provide require rules for this target
  void WriteTargetRequiresRules();

  // write the clean rules for this target
  void WriteTargetCleanRules();

  // write the depend rules for this target
  void WriteTargetDependRules();

  // write rules for Mac OS X Application Bundle content.
  struct MacOSXContentGeneratorType
    : cmOSXBundleGenerator::MacOSXContentGeneratorType
  {
    MacOSXContentGeneratorType(cmMakefileTargetGenerator* gen)
      : Generator(gen)
    {
    }

    void operator()(cmSourceFile const& source,
                    const char* pkgloc) CM_OVERRIDE;

  private:
    cmMakefileTargetGenerator* Generator;
  };
  friend struct MacOSXContentGeneratorType;

  // write the rules for an object
  void WriteObjectRuleFiles(cmSourceFile const& source);

  // write the build rule for an object
  void WriteObjectBuildFile(std::string& obj, const std::string& lang,
                            cmSourceFile const& source,
                            std::vector<std::string>& depends);

  // write the depend.make file for an object
  void WriteObjectDependRules(cmSourceFile const& source,
                              std::vector<std::string>& depends);

  // write the build rule for a custom command
  void GenerateCustomRuleFile(cmCustomCommandGenerator const& ccg);

  // write a rule to drive building of more than one output from
  // another rule
  void GenerateExtraOutput(const char* out, const char* in,
                           bool symbolic = false);

  void MakeEchoProgress(cmLocalUnixMakefileGenerator3::EchoProgress&) const;

  // write out the variable that lists the objects for this target
  void WriteObjectsVariable(std::string& variableName,
                            std::string& variableNameExternal,
                            bool useWatcomQuote);
  void WriteObjectsStrings(std::vector<std::string>& objStrings,
                           std::string::size_type limit = std::string::npos);

  // write the driver rule to build target outputs
  void WriteTargetDriverRule(const std::string& main_output, bool relink);

  void DriveCustomCommands(std::vector<std::string>& depends);

  // append intertarget dependencies
  void AppendTargetDepends(std::vector<std::string>& depends);

  // Append object file dependencies.
  void AppendObjectDepends(std::vector<std::string>& depends);

  // Append link rule dependencies (objects, etc.).
  void AppendLinkDepends(std::vector<std::string>& depends);

  // Lookup the link rule for this target.
  std::string GetLinkRule(const std::string& linkRuleVar);

  /** Create a script to hold link rules and a command to invoke the
      script at build time.  */
  void CreateLinkScript(const char* name,
                        std::vector<std::string> const& link_commands,
                        std::vector<std::string>& makefile_commands,
                        std::vector<std::string>& makefile_depends);

  cmLinkLineComputer* CreateLinkLineComputer(
    cmOutputConverter* outputConverter, cmStateDirectory const& stateDir);

  /** Create a response file with the given set of options.  Returns
      the relative path from the target build working directory to the
      response file name.  */
  std::string CreateResponseFile(const char* name, std::string const& options,
                                 std::vector<std::string>& makefile_depends);

  bool CheckUseResponseFileForObjects(std::string const& l) const;
  bool CheckUseResponseFileForLibraries(std::string const& l) const;

  /** Create list of flags for link libraries. */
  void CreateLinkLibs(cmLinkLineComputer* linkLineComputer,
                      std::string& linkLibs, bool useResponseFile,
                      std::vector<std::string>& makefile_depends);

  /** Create lists of object files for linking and cleaning.  */
  void CreateObjectLists(bool useLinkScript, bool useArchiveRules,
                         bool useResponseFile, std::string& buildObjs,
                         std::vector<std::string>& makefile_depends,
                         bool useWatcomQuote);

  /** Add commands for generate def files */
  void GenDefFile(std::vector<std::string>& real_link_commands);

  void AddIncludeFlags(std::string& flags,
                       const std::string& lang) CM_OVERRIDE;

  virtual void CloseFileStreams();
  cmLocalUnixMakefileGenerator3* LocalGenerator;
  cmGlobalUnixMakefileGenerator3* GlobalGenerator;

  enum CustomCommandDriveType
  {
    OnBuild,
    OnDepends,
    OnUtility
  };
  CustomCommandDriveType CustomCommandDriver;

  // the full path to the build file
  std::string BuildFileName;
  std::string BuildFileNameFull;

  // the full path to the progress file
  std::string ProgressFileNameFull;
  unsigned long NumberOfProgressActions;
  bool NoRuleMessages;

  // the path to the directory the build file is in
  std::string TargetBuildDirectory;
  std::string TargetBuildDirectoryFull;

  // the stream for the build file
  cmGeneratedFileStream* BuildFileStream;

  // the stream for the flag file
  std::string FlagFileNameFull;
  cmGeneratedFileStream* FlagFileStream;
  class StringList : public std::vector<std::string>
  {
  };
  std::map<std::string, StringList> FlagFileDepends;

  // the stream for the info file
  std::string InfoFileNameFull;
  cmGeneratedFileStream* InfoFileStream;

  // files to clean
  std::vector<std::string> CleanFiles;

  // objects used by this target
  std::vector<std::string> Objects;
  std::vector<std::string> ExternalObjects;

  // Set of object file names that will be built in this directory.
  std::set<std::string> ObjectFiles;

  // Set of extra output files to be driven by the build.
  std::set<std::string> ExtraFiles;

  typedef std::map<std::string, std::string> MultipleOutputPairsType;
  MultipleOutputPairsType MultipleOutputPairs;
  bool WriteMakeRule(std::ostream& os, const char* comment,
                     const std::vector<std::string>& outputs,
                     const std::vector<std::string>& depends,
                     const std::vector<std::string>& commands,
                     bool in_help = false);

  // Target name info.
  std::string TargetNameOut;
  std::string TargetNameSO;
  std::string TargetNameReal;
  std::string TargetNameImport;
  std::string TargetNamePDB;

  // Mac OS X content info.
  std::set<std::string> MacContentFolders;
  cmOSXBundleGenerator* OSXBundleGenerator;
  MacOSXContentGeneratorType* MacOSXContentGenerator;
};

#endif
