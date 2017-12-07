/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#ifndef cmRulePlaceholderExpander_h
#define cmRulePlaceholderExpander_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <map>
#include <string>

class cmOutputConverter;

class cmRulePlaceholderExpander
{
public:
  cmRulePlaceholderExpander(
    std::map<std::string, std::string> const& compilers,
    std::map<std::string, std::string> const& variableMappings,
    std::string const& compilerSysroot, std::string const& linkerSysroot);

  void SetTargetImpLib(std::string const& targetImpLib)
  {
    this->TargetImpLib = targetImpLib;
  }

  // Create a struct to hold the varibles passed into
  // ExpandRuleVariables
  struct RuleVariables
  {
    RuleVariables();
    const char* CMTargetName;
    const char* CMTargetType;
    const char* TargetPDB;
    const char* TargetCompilePDB;
    const char* TargetVersionMajor;
    const char* TargetVersionMinor;
    const char* Language;
    const char* Objects;
    const char* Target;
    const char* LinkLibraries;
    const char* Source;
    const char* AssemblySource;
    const char* PreprocessedSource;
    const char* Output;
    const char* Object;
    const char* ObjectDir;
    const char* ObjectFileDir;
    const char* Flags;
    const char* ObjectsQuoted;
    const char* SONameFlag;
    const char* TargetSOName;
    const char* TargetInstallNameDir;
    const char* LinkFlags;
    const char* Manifests;
    const char* LanguageCompileFlags;
    const char* Defines;
    const char* Includes;
    const char* DependencyFile;
    const char* FilterPrefix;
  };

  // Expand rule variables in CMake of the type found in language rules
  void ExpandRuleVariables(cmOutputConverter* outputConverter,
                           std::string& string,
                           const RuleVariables& replaceValues);

  // Expand rule variables in a single string
  std::string ExpandRuleVariable(cmOutputConverter* outputConverter,
                                 std::string const& variable,
                                 const RuleVariables& replaceValues);

private:
  std::string TargetImpLib;

  std::map<std::string, std::string> Compilers;
  std::map<std::string, std::string> VariableMappings;
  std::string CompilerSysroot;
  std::string LinkerSysroot;
};

#endif
